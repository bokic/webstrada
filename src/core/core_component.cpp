// ColdFusion Component (CFC) runtime: instance creation, method dispatch,
// access control, CreateObject / `new` resolution.
//
// A compiled .cfc is represented by a ComponentInfo (definition) whose JIT
// method bodies use the component_method_entry_fn signature and whose top-level
// body uses component_body_fn. cf_component_instantiate builds a live
// ComponentInstance (this scope + variables scope), runs the extends chain's
// construction bodies then this component's body, and returns a Component
// cfvariant whose StructData IS the this scope.

#include "core_internal.h"

#include <webstrada/component.h>
#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include "../cftags/common.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <sys/stat.h>

namespace webstrada {

ComponentInfo *component_info_retain(ComponentInfo *info)
{
    if (info) info->refs++;
    return info;
}

void component_info_release(ComponentInfo *info)
{
    if (!info) return;
    if (--info->refs > 0) return;
    component_info_release(info->parent);
    for (auto *p : info->interfaceParents) component_info_release(p);
    for (auto *p : info->interfaces) component_info_release(p);
    delete info;
}

ComponentInstance *component_instance_retain(ComponentInstance *inst)
{
    if (inst) inst->refs++;
    return inst;
}

void component_instance_release(ComponentInstance *inst)
{
    if (!inst) return;
    if (--inst->refs > 0) return;
    component_info_release(inst->info);
    delete inst->thisScope;
    delete inst->variablesScope;
    delete inst;
}

thread_local ComponentInfo *g_currentMethodOwnerInfo = nullptr;

int findMethodInInfo(ComponentInfo *info, const std::string &upper, ComponentInfo *&owner)
{
    if (!info) return -1;
    for (size_t i = 0; i < info->methods.size(); i++) {
        if (info->methods[i].name == upper) {
            owner = info;
            return static_cast<int>(i);
        }
    }
    if (info->parent) return findMethodInInfo(info->parent, upper, owner);
    return -1;
}

} // namespace webstrada

namespace cfml {

using namespace webstrada;

// Public wrapper so built-in functions outside this translation unit can release
// a ComponentInfo acquired via cf_component_load().
void cf_component_info_release(ComponentInfo *info)
{
    component_info_release(info);
}

// Push a component-method/body call context. `localScope` is the function-local
// scope; the compiled body's `variables` is the component's variables scope, so
// the ctx records the component context explicitly (this scope + instance).
void cf_component_udf_begin(cfvariant *localScope, cfvariant *variablesScope,
                            cfvariant *thisScope, void *component)
{
    UdfCallCtx ctx;
    ctx.localScope = localScope;
    ctx.parentScope = variablesScope;
    ctx.thisScope = thisScope;
    ctx.component = static_cast<ComponentInstance*>(component);
    ctx.componentInfo = g_currentMethodOwnerInfo ? g_currentMethodOwnerInfo : (ctx.component ? ctx.component->info : nullptr);
    g_currentMethodOwnerInfo = nullptr;
    g_udfCtx.push_back(std::move(ctx));
}

// Builds a Component cfvariant from a live instance, sharing the this scope's
// StructData so struct introspection / SerializeJSON / member access work.
cfvariant *makeComponentVariant(ComponentInstance *inst, bool retain);

// The `this` value inside a component method/body: a temporary Component
// variant wrapping the current instance (CF's `this` is the object itself).
cfvariant *componentThisValue(ComponentInstance *inst);

// ---- Component definition loading (CreateObject / <cfobject> / `new`) ----

// Backtracking search helper for dot-paths where segments could contain literal
// dots (e.g. "MangoBlog_1.4.3").
static bool searchDotPathOnDisk(const std::vector<std::string> &parts, size_t idx,
                                const std::string &currPath, std::string &outResolved)
{
    if (idx == parts.size()) {
        for (const char *ext : {"", ".cfc", ".cfml"}) {
            std::string full = currPath + ext;
            struct stat st;
            if (stat(full.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
                outResolved = currPath;
                return true;
            }
        }
        return false;
    }

    // 1. As a new path segment with '/'
    std::string candSlash = currPath.empty() ? parts[idx] : (currPath + "/" + parts[idx]);
    if (searchDotPathOnDisk(parts, idx + 1, candSlash, outResolved)) {
        return true;
    }

    // 2. Joined with the previous segment with '.'
    if (!currPath.empty()) {
        std::string candDot = currPath + "." + parts[idx];
        if (searchDotPathOnDisk(parts, idx + 1, candDot, outResolved)) {
            return true;
        }
    }

    return false;
}

// Resolve a component dot/relative path against the include runtime (current
// template dir for relative, falling back to web root for dotted paths, or
// web root for root-relative /... paths), matching Adobe ColdFusion component lookup.
static bool componentResolvePath(const std::string &path, std::string &resolved)
{
    std::string p = path;
    for (auto &c : p) {
        if (c == '\\') c = '/';
    }

    auto tryMapping = [](const std::string &inPath, std::string &outRes) -> bool {
        if (cfml::app_mappings_resolve(inPath, outRes)) {
            // If the resolved path doesn't end with .cfc / .cfml, check if appending .cfc exists or default to .cfc
            if (!(outRes.size() >= 4 && outRes.compare(outRes.size() - 4, 4, ".cfc") == 0) &&
                !(outRes.size() >= 5 && outRes.compare(outRes.size() - 5, 5, ".cfml") == 0)) {
                std::string withCfc = outRes + ".cfc";
                struct stat st;
                if (stat(withCfc.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
                    outRes = withCfc;
                    return true;
                }
                std::string withCfml = outRes + ".cfml";
                if (stat(withCfml.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
                    outRes = withCfml;
                    return true;
                }
            }
            return true;
        }
        return false;
    };

    // If it's a dotted path without slashes (e.g. org.mangoblog.utilities.Preferences),
    // convert dots to slashes for mapping lookup (e.g. org/mangoblog/utilities/Preferences).
    if (p.find('/') == std::string::npos && p.find('.') != std::string::npos) {
        std::string slashPath = p;
        for (auto &c : slashPath) {
            if (c == '.') c = '/';
        }
        if (tryMapping(slashPath, resolved)) {
            return true;
        }
    }

    // Check Application.cfc this.mappings directly
    if (tryMapping(p, resolved)) {
        return true;
    }

    IncludeRuntime *rt = cfml::include_context();
    if (!rt) return false;

    std::filesystem::path input(p);
    if (input.is_absolute()) {
        resolved = input.lexically_normal().string();
        return true;
    }

    if (!p.empty() && p[0] == '/') {
        if (rt->webRoot.empty()) return false;
        std::filesystem::path base(rt->webRoot);
        resolved = (base / input.relative_path()).lexically_normal().string();
        return true;
    }

    // Split dotted path into parts to support directory/file names with literal dots
    bool hadSlash = (p.find('/') != std::string::npos);
    std::vector<std::string> parts;
    if (!hadSlash && p.find('.') != std::string::npos) {
        std::string cur;
        for (char c : p) {
            if (c == '.') {
                parts.push_back(cur);
                cur.clear();
            } else {
                cur += c;
            }
        }
        parts.push_back(cur);
    }

    // Try relative to currentPath first (if exists on disk)
    if (!rt->currentPath.empty()) {
        std::filesystem::path cur(rt->currentPath);
        std::filesystem::path base = cur.has_parent_path() ? cur.parent_path() : std::filesystem::path("");
        if (!parts.empty()) {
            std::string match;
            if (searchDotPathOnDisk(parts, 0, base.string(), match)) {
                resolved = match;
                return true;
            }
        }
        std::string cand = (base / input).lexically_normal().string();
        std::string candWithExt = cand;
        if (!(candWithExt.size() >= 4 && candWithExt.compare(candWithExt.size() - 4, 4, ".cfc") == 0) &&
            !(candWithExt.size() >= 5 && candWithExt.compare(candWithExt.size() - 5, 5, ".cfml") == 0)) {
            candWithExt += ".cfc";
        }
        struct stat st;
        if (stat(candWithExt.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            resolved = cand;
            return true;
        }
        // If webRoot is not available, default to relative
        if (rt->webRoot.empty()) {
            resolved = cand;
            return true;
        }
    }

    // Fall back to web root (e.g. dot-paths like components.utilities.PreferencesFile or full root-relative dot-paths)
    if (!rt->webRoot.empty()) {
        std::filesystem::path base(rt->webRoot);
        if (!parts.empty()) {
            std::string match;
            if (searchDotPathOnDisk(parts, 0, base.string(), match)) {
                resolved = match;
                return true;
            }
        }
        std::string cand = (base / input).lexically_normal().string();
        std::string candWithExt = cand;
        if (!(candWithExt.size() >= 4 && candWithExt.compare(candWithExt.size() - 4, 4, ".cfc") == 0) &&
            !(candWithExt.size() >= 5 && candWithExt.compare(candWithExt.size() - 5, 5, ".cfml") == 0)) {
            candWithExt += ".cfc";
        }
        struct stat st;
        if (stat(candWithExt.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            resolved = cand;
            return true;
        }
        // If neither exists on disk, default to relative to currentPath if present, else webRoot
        if (!rt->currentPath.empty()) {
            std::filesystem::path cur(rt->currentPath);
            std::filesystem::path base = cur.has_parent_path() ? cur.parent_path() : std::filesystem::path("");
            resolved = (base / input).lexically_normal().string();
        } else {
            resolved = cand;
        }
        return true;
    }

    return false;
}

// Resolve an `extends` value (a relative .cfc path or a dot path) against a
// base directory into an absolute .cfc path.
static bool resolveParentPath(const std::string &extends, const std::string &baseDir,
                              std::string &resolved)
{
    std::string p = extends;
    for (auto &c : p) {
        if (c == '\\') c = '/';
    }
    // Dot paths (foo.bar / a.b.comp) map to subdirectories.
    if (p.find('/') == std::string::npos) {
        for (auto &c : p) {
            if (c == '.') c = '/';
        }
        p += ".cfc";
    } else if (!(p.size() >= 4 && p.compare(p.size() - 4, 4, ".cfc") == 0) &&
               !(p.size() >= 5 && p.compare(p.size() - 5, 5, ".cfml") == 0)) {
        p += ".cfc";
    }
    // Check Application.cfc this.mappings first
    if (cfml::app_mappings_resolve(p, resolved)) {
        return true;
    }
    std::filesystem::path base(baseDir);
    std::filesystem::path input(p);
    if (input.is_absolute()) {
        resolved = input.lexically_normal().string();
    } else {
        resolved = (base / input).lexically_normal().string();
    }
    return true;
}

// The web-root-relative dot name of a resolved .cfc path (e.g.
// /app/tmp_iface/dog.cfc -> "tmp_iface.dog"); falls back to the file base name
// when the path is not under the web root.
static std::string componentFullName(const std::string &resolvedPath)
{
    IncludeRuntime *rt = cfml::include_context();
    std::string p = resolvedPath;
    if (rt && !rt->webRoot.empty()) {
        std::string wr = rt->webRoot;
        while (!wr.empty() && wr.back() == '/') wr.pop_back();
        if (p.size() > wr.size() && p.compare(0, wr.size(), wr) == 0 &&
            (wr.empty() || p[wr.size()] == '/')) {
            p = p.substr(wr.size() + 1);
            size_t dot = p.rfind(".cfc");
            if (dot != std::string::npos && dot + 4 == p.size()) p = p.substr(0, dot);
            else {
                dot = p.rfind(".cfml");
                if (dot != std::string::npos && dot + 5 == p.size()) p = p.substr(0, dot);
            }
            for (auto &c : p) {
                if (c == '/' || c == '\\') c = '.';
            }
            return p;
        }
    }
    // Not under the web root: use the file base name.
    std::string base = resolvedPath;
    size_t slash = base.find_last_of('/');
    if (slash != std::string::npos) base = base.substr(slash + 1);
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    return base;
}

// Resolve an `implements` / interface-`extends` value (a dot path or relative
// .cfc path) into an absolute .cfc path, first against the component's own
// directory then against the web root (matching CF's interface resolution).
static bool resolveInterfacePath(const std::string &value, const std::string &cfcPath,
                                 std::string &resolved)
{
    std::filesystem::path cf(cfcPath);
    std::string baseDir = cf.has_parent_path() ? cf.parent_path().string() : std::string(".");
    if (resolveParentPath(value, baseDir, resolved)) {
        struct stat st;
        if (stat(resolved.c_str(), &st) == 0 && S_ISREG(st.st_mode)) return true;
    }
    // Fall back to the web root.
    IncludeRuntime *rt = cfml::include_context();
    if (rt && !rt->webRoot.empty()) {
        std::string p = value;
        for (auto &c : p) {
            if (c == '\\') c = '/';
        }
        if (p.find('/') == std::string::npos) {
            for (auto &c : p) {
                if (c == '.') c = '/';
            }
            p += ".cfc";
        } else if (!(p.size() >= 4 && p.compare(p.size() - 4, 4, ".cfc") == 0) &&
                   !(p.size() >= 5 && p.compare(p.size() - 5, 5, ".cfml") == 0)) {
            p += ".cfc";
        }
        std::filesystem::path base(rt->webRoot);
        std::filesystem::path input(p);
        if (input.is_absolute()) {
            resolved = input.lexically_normal().string();
        } else {
            resolved = (base / input).lexically_normal().string();
        }
        struct stat st;
        if (stat(resolved.c_str(), &st) == 0 && S_ISREG(st.st_mode)) return true;
    }
    return false;
}

// "Could not find the ColdFusion component or interface X." — the name is the
// web-root-relative dot path of the attempted resolution.
[[noreturn]] static void throwInterfaceNotFound(const std::string &value, const std::string &cfcPath)
{
    std::string attempted;
    std::filesystem::path cf(cfcPath);
    std::string baseDir = cf.has_parent_path() ? cf.parent_path().string() : std::string(".");
    std::string p;
    if (resolveParentPath(value, baseDir, p)) {
        attempted = componentFullName(p);
    } else {
        attempted = value;
    }
    throw webstrada::exception("Application",
        ("Could not find the ColdFusion component or interface " + attempted + ".").c_str(),
        webstrada::string("Ensure that the name is correct and that the component or interface exists."));
}

// The interface's method list including its extends parents (parents first).
static void collectInterfaceMethods(ComponentInfo *iface, std::vector<ComponentMethod*> &out)
{
    if (!iface) return;
    for (ComponentInfo *p : iface->interfaceParents) collectInterfaceMethods(p, out);
    for (auto &m : iface->methods) out.push_back(&m);
}

// Whether the component's method satisfies the interface method's signature
// (CF's UDFMethod.verifyMethodImplementation, see BUGS_CF.md for notes).
static void verifyInterfaceMethod(ComponentInfo *comp, ComponentMethod &compMethod,
                                  ComponentMethod &ifaceMethod,
                                  const std::string &compName, const std::string &ifaceName)
{
    // Return type must be identical (or a covariant subclass; scalar types must
    // match exactly — CF's compareUDFAttributes). null/"any" are equivalent.
    auto typeMatch = [](const std::string &a, const std::string &b) -> bool {
        auto isAny = [](const std::string &s) {
            return s.empty() || s == "any" || s == "Any" || s == "ANY";
        };
        if (isAny(a) && isAny(b)) return true;
        return a == b;
    };
    if (!typeMatch(compMethod.returnType, ifaceMethod.returnType)) {
        std::string ifaceType = ifaceMethod.returnType.empty() ? "any" : ifaceMethod.returnType;
        std::string compType = compMethod.returnType.empty() ? "any" : compMethod.returnType;
        std::string detail = "The " + ifaceMethod.declaredName + " function does not specify  covariant return type in  the " +
            compName + " ColdFusion component and the " + ifaceName + " ColdFusion interface.  " + ifaceName +
            " ColdFusion interface declared the type as " + ifaceType + ", whereas " + compName +
             " ColdFusion component specified it as " + compType +
             ".  It has to be either the same type or subclass type of the return type declared in overridden component.";
        throw webstrada::exception("Application", "Return type mismatch.", webstrada::string(detail.c_str()));
    }
    if (compMethod.paramTypes.size() != ifaceMethod.paramTypes.size()) {
        std::string detail = "The " + ifaceMethod.declaredName + " function does not specify the same arguments or arguments in the same order in  the " +
            compName + " ColdFusion component and the " + ifaceName + " ColdFusion interface.";
        throw webstrada::exception("Application", "Function argument mismatch.", webstrada::string(detail.c_str()));
    }
    for (size_t i = 0; i < ifaceMethod.paramTypes.size(); i++) {
        std::string argName = (i < ifaceMethod.paramNames.size() && !ifaceMethod.paramNames[i].empty())
            ? ifaceMethod.paramNames[i] : std::string("argument");
        // Data type (positional).
        if (!typeMatch(compMethod.paramTypes[i], ifaceMethod.paramTypes[i])) {
            std::string ifaceType = ifaceMethod.paramTypes[i].empty() ? "any" : ifaceMethod.paramTypes[i];
            std::string compType = compMethod.paramTypes[i].empty() ? "any" : compMethod.paramTypes[i];
            std::string detail = "The " + ifaceMethod.declaredName + " function does not specify the same data type for the " + argName +
                " argument.  " + ifaceName + " ColdFusion interface declared data type as " + ifaceType + ", whereas " + compName +
                " ColdFusion component    specified it as " + compType + ".";
            throw webstrada::exception("Application", "Data type mismatch.", webstrada::string(detail.c_str()));
        }
        // Required flag.
        bool ifaceReq = i < ifaceMethod.paramRequired.size() && ifaceMethod.paramRequired[i];
        bool compReq = i < compMethod.paramRequired.size() && compMethod.paramRequired[i];
        if (compReq != ifaceReq) {
            std::string detail = "The " + ifaceMethod.declaredName + " function does not specify the same required value for the " + argName +
                " argument in  the " + compName + " ColdFusion component and the " + ifaceName + " ColdFusion interface.";
            throw webstrada::exception("Application", "Required argument mismatch.", webstrada::string(detail.c_str()));
        }
        // Default value.
        std::string ifaceDef;
        if (i < ifaceMethod.params.size() && ifaceMethod.params[i].defaultValue.constData()) {
            ifaceDef = ifaceMethod.params[i].defaultValue.constData();
        }
        std::string compDef;
        if (i < compMethod.params.size() && compMethod.params[i].defaultValue.constData()) {
            compDef = compMethod.params[i].defaultValue.constData();
        }
        if (!ifaceDef.empty() && !compDef.empty()) {
            bool match = false;
            if (compDef == ifaceDef) match = true;
            else {
                std::string dt = compMethod.paramTypes[i].empty() ? "any" : compMethod.paramTypes[i];
                std::string dtl = dt;
                for (auto &c : dtl) c = (char)tolower((unsigned char)c);
                if (dtl == "numeric" || dtl == "boolean" || dtl == "date" || dtl == "any") {
                    try {
                        if (strtod(compDef.c_str(), nullptr) == strtod(ifaceDef.c_str(), nullptr)) match = true;
                    } catch (...) {}
                }
            }
            if (!match) {
                std::string detail = "The " + ifaceMethod.declaredName + " function does not specify the same default value for the " + argName +
                    " argument in  the " + compName + " ColdFusion component and the " + ifaceName + " ColdFusion interface.";
                throw webstrada::exception("Application", "Argument default value mismatch.", webstrada::string(detail.c_str()));
            }
        }
    }
}

// Validates that the component implements the interface (recursively through
// the interface's extends parents), matching CF's interface check.
static void validateAgainstInterface(ComponentInfo *comp, ComponentInfo *iface,
                                     const std::string &compName, const std::string &ifaceName)
{
    std::vector<ComponentMethod*> ifaceMethods;
    collectInterfaceMethods(iface, ifaceMethods);
    for (ComponentMethod *im : ifaceMethods) {
        ComponentInfo *owner = nullptr;
        int idx = findMethodInInfo(comp, im->name, owner);
        bool accessible = idx >= 0;
        if (accessible && owner) {
            std::string acc = owner->methods[idx].access;
            for (auto &c : acc) c = (char)tolower((unsigned char)c);
            if (acc == "private") accessible = false;
        }
        if (!accessible) {
            std::string detail = "The " + im->declaredName + " method is not implemented by the component or it is declared as private.";
            throw webstrada::exception("Application",
                ("CFC " + compName + " does not implement the interface " + ifaceName + ".").c_str(),
                webstrada::string(detail.c_str()));
        }
        verifyInterfaceMethod(comp, owner->methods[idx], *im, compName, ifaceName);
    }
    for (ComponentInfo *p : iface->interfaceParents) {
        validateAgainstInterface(comp, p, compName, ifaceName);
    }
}

// Resolve and validate the component's `implements` interfaces. Runs once per
// ComponentInfo (guard `validationDone`), lazily at first load like CF.
static void resolveAndValidateInterfaces(ComponentInfo *info)
{
    if (info->validationDone) return;
    info->validationDone = true;
    if (info->isInterface || info->implementsText.empty()) return;

    std::string compName = componentFullName(info->cfcPath);
    // Split the comma-delimited implements list.
    std::vector<std::string> names;
    std::string cur;
    for (char c : info->implementsText) {
        if (c == ',') {
            std::string t = cur;
            while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t.erase(t.begin());
            while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.pop_back();
            if (!t.empty()) names.push_back(t);
            cur.clear();
        } else {
            cur += c;
        }
    }
    std::string t = cur;
    while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t.erase(t.begin());
    while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.pop_back();
    if (!t.empty()) names.push_back(t);

    for (const auto &n : names) {
        std::string resolved;
        if (!resolveInterfacePath(n, info->cfcPath, resolved)) {
            throwInterfaceNotFound(n, info->cfcPath);
        }
        IncludeRuntime *rt = cfml::include_context();
        std::string prevCur = rt ? rt->currentPath : std::string();
        if (rt && !info->cfcPath.empty()) rt->currentPath = info->cfcPath;
        ComponentInfo *iface = cf_component_load(resolved.c_str());
        if (rt) rt->currentPath = prevCur;
        if (!iface) {
            throwInterfaceNotFound(n, info->cfcPath);
        }
        struct IfaceGuard {
            ComponentInfo *i;
            ~IfaceGuard() { if (i) component_info_release(i); }
        } ifaceGuard{iface};
        if (!iface->isInterface) {
            std::string ifaceName = componentFullName(resolved);
            std::string detail = "The " + compName + " ColdFusion component   cannot implement the " + ifaceName +
                "   ColdFusion component. It can only implement interfaces.";
            throw webstrada::exception("Application", "Component cannot implement component.", webstrada::string(detail.c_str()));
        }
        std::string ifaceName = iface->fullName.empty() ? componentFullName(resolved) : iface->fullName;
        validateAgainstInterface(info, iface, compName, ifaceName);
        info->interfaces.push_back(iface);
        ifaceGuard.i = nullptr;  // borrowed; the child's release drops the loader retain
    }
}

ComponentInfo *cf_component_load(const char *path)
{
    if (!path || !path[0]) return nullptr;
    IncludeRuntime *rt = cfml::include_context();
    if (!rt || !rt->componentLoader) {
        throw webstrada::exception("component", "Components are not available in this context.");
    }
    std::string resolved;
    if (!componentResolvePath(path, resolved)) {
        throw webstrada::exception("component",
            webstrada::string(("Could not resolve the component " + std::string(path) + ".").c_str()));
    }
    // A bare component name ("comp1", "foo.bar.comp") has no extension; CF
    // appends .cfc.
    if (!(resolved.size() >= 4 && resolved.compare(resolved.size() - 4, 4, ".cfc") == 0) &&
        !(resolved.size() >= 5 && resolved.compare(resolved.size() - 5, 5, ".cfml") == 0)) {
        resolved += ".cfc";
    }
    ComponentInfo *info = rt->componentLoader(resolved.c_str(), rt->componentLoaderOpaque);
    if (!info && !g_importPaths.empty()) {
        // <cfimport path="..."> fallback: a bare component name that missed the
        // plain relative resolution is tried against the registered import
        // paths (a specific dotted path whose last segment matches, or a
        // wildcard "prefix.*" which appends ".Name"), like CF's importList.
        std::string orig = path;
        for (auto &c : orig) {
            if (c == '\\') c = '/';
        }
        bool hadDot = orig.find('/') != std::string::npos || orig.find('.') != std::string::npos;
        if (!hadDot) {
            std::string candidate;
            for (const auto &imp : g_importPaths) {
                std::string ip = imp;
                if (!ip.empty() && ip.back() == '*') {
                    ip.pop_back();
                    if (!ip.empty() && ip.back() == '.') ip.pop_back();
                    candidate = ip + "." + orig;
                } else {
                    size_t lastDot = ip.find_last_of('.');
                    std::string seg = (lastDot == std::string::npos) ? ip : ip.substr(lastDot + 1);
                    if (seg == orig) candidate = ip;
                    else continue;
                }
                std::string candPath = candidate;
                for (auto &c : candPath) {
                    if (c == '.') c = '/';
                }
                std::string candResolved;
                if (componentResolvePath(candPath, candResolved)) {
                    if (!(candResolved.size() >= 4 && candResolved.compare(candResolved.size() - 4, 4, ".cfc") == 0) &&
                        !(candResolved.size() >= 5 && candResolved.compare(candResolved.size() - 5, 5, ".cfml") == 0)) {
                        candResolved += ".cfc";
                    }
                    ComponentInfo *cand = rt->componentLoader(candResolved.c_str(), rt->componentLoaderOpaque);
                    if (cand) {
                        info = cand;
                        resolved = candResolved;
                        break;
                    }
                }
            }
        }
    }
    if (!info) return nullptr;
    // `info` was retained by the loader (get_component); if any resolution
    // below throws, release that retain so the definition (and its LLVM code)
    // is freed instead of leaking.
    struct InfoGuard {
        ComponentInfo *i;
        ~InfoGuard() { if (i) component_info_release(i); }
    } infoGuard{info};
    if (info->fullName.empty()) info->fullName = componentFullName(resolved);
    {
        // Keep the original createObject/new path verbatim (CF echoes it in the
        // "Cannot create interface" error), unless it was a resolved file path.
        std::string p = path;
        if (!(p.size() >= 4 && p.compare(p.size() - 4, 4, ".cfc") == 0) &&
            !(p.size() >= 5 && p.compare(p.size() - 5, 5, ".cfml") == 0)) {
            info->displayPath = p;
        }
        if (info->displayPath.empty()) info->displayPath = info->fullName;
    }
    // Resolve the extends chain lazily (the parent .cfc is compiled on demand
    // through the same loader, which caches).
    if (!info->parent && !info->extendsPath.empty()) {
        std::string parentPath;
        std::filesystem::path cf(info->cfcPath);
        std::string baseDir = cf.has_parent_path() ? cf.parent_path().string() : std::string(".");
        if (resolveParentPath(info->extendsPath, baseDir, parentPath)) {
            std::string prevCur = rt ? rt->currentPath : std::string();
            if (rt && !info->cfcPath.empty()) rt->currentPath = info->cfcPath;
            ComponentInfo *parent = cf_component_load(parentPath.c_str());
            if (rt) rt->currentPath = prevCur;
            if (parent) {
                // `parent` is borrowed: the child's component_info_release
                // chains to info->parent and drops the loader retain.
                info->parent = parent;
            } else {
                throw webstrada::exception("component",
                    webstrada::string(("The extends path " + info->extendsPath + " could not be found.").c_str()));
            }
        }
    }
    // Interface: resolve the `extends` interfaces (comma-delimited) lazily.
    if (info->isInterface && info->interfaceParents.empty() && !info->extendsList.empty()) {
        for (const auto &ename : info->extendsList) {
            std::string parentPath;
            if (!resolveInterfacePath(ename, info->cfcPath, parentPath)) {
                throwInterfaceNotFound(ename, info->cfcPath);
            }
            std::string prevCur = rt ? rt->currentPath : std::string();
            if (rt && !info->cfcPath.empty()) rt->currentPath = info->cfcPath;
            ComponentInfo *piface = cf_component_load(parentPath.c_str());
            if (rt) rt->currentPath = prevCur;
            if (!piface) {
                throwInterfaceNotFound(ename, info->cfcPath);
            }
            if (!piface->isInterface) {
                std::string ifaceName = componentFullName(info->cfcPath);
                std::string pName = componentFullName(parentPath);
                component_info_release(piface);
                throw webstrada::exception("Application",
                    ("Interface " + ifaceName + " cannot extend the component " + pName + ".").c_str(),
                    webstrada::string("ColdFusion interface cannot extend ColdFusion component."));            }
            info->interfaceParents.push_back(piface);
        }
    }
    // Component: resolve and validate the `implements` interfaces.
    if (!info->isInterface && !info->implementsText.empty()) {
        resolveAndValidateInterfaces(info);
    }
    infoGuard.i = nullptr;  // success: transfer ownership to the caller
    return info;
}


static int findMethodInInstance(ComponentInstance *inst, const std::string &upper, ComponentInfo *&owner)
{
    if (!inst || !inst->info) return -1;
    return findMethodInInfo(inst->info, upper, owner);
}

// Whether a method is externally callable (public/package/remote).
static bool methodExternallyCallable(const ComponentMethod &m)
{
    std::string a = m.access;
    for (auto &c : a) c = (char)tolower((unsigned char)c);
    return a.empty() || a == "public" || a == "package" || a == "remote";
}


static void runConstructionBody(ComponentInfo *info, ComponentInstance *inst,
                                string &out, void *cgi, void *server, void *cookie,
                                void *application, void *session, void *url, void *form)
{
    if (!info->body) return;
    auto bodyFn = reinterpret_cast<component_body_fn>(info->body);
    // Push a component context so `this.x` / unqualified names resolve against
    // the instance's scopes during construction.
    cfvariant *localScope = new cfvariant(cfvariant::Struct);
    cf_register_temp(localScope);
    UdfCallCtx ctx;
    ctx.localScope = localScope;
    ctx.parentScope = inst->variablesScope;
    ctx.thisScope = inst->thisScope;
    ctx.component = inst;
    g_udfCtx.push_back(std::move(ctx));

    IncludeRuntime *rt = cfml::include_context();
    std::string prevPath;
    if (rt) {
        prevPath = rt->currentPath;
        if (!info->cfcPath.empty()) {
            rt->currentPath = info->cfcPath;
        }
    }

    try {
        bodyFn(&out, cgi, server, cookie, application, session, url, form,
               inst->variablesScope, inst->thisScope);
    } catch (const webstrada::exit_exception &) {
        // <cfexit> in the construction body stops it there; the instantiation
        // still completes (verified on CF: `new C()` with an `exit;` in the
        // body returns the instance and the page continues).
    } catch (...) {
        if (rt) rt->currentPath = prevPath;
        g_udfCtx.pop_back();
        throw;
    }
    if (rt) rt->currentPath = prevPath;
    g_udfCtx.pop_back();
}

cfvariant *cf_component_instantiate(ComponentInfo *info, cfvariant *variables,
                                    string *out, void *cgi, void *server, void *cookie,
                                    void *application, void *session, void *url, void *form)
{
    (void)variables;
    if (!info) throw webstrada::exception("component", "Attempt to instantiate an undefined component.");
    if (info->isInterface) {
        std::string ifaceName = info->displayPath.empty() ? (info->fullName.empty() ? info->name : info->fullName) : info->displayPath;
        throw webstrada::exception("Application", "Cannot create interface.",
            webstrada::string(("Cannot create the  " + ifaceName + " ColdFusion interface.").c_str()));
    }
    auto *inst = new ComponentInstance();
    inst->info = component_info_retain(info);
    inst->thisScope = new cfvariant(cfvariant::Struct);
    inst->variablesScope = new cfvariant(cfvariant::Struct);

    try {
        // Extends: run the parent's construction bodies first (outermost
        // parent first) so the parent's this./variables. assignments land in
        // this instance's scopes, then this component's body.
        if (info->parent) {
            std::vector<ComponentInfo*> chain;
            for (ComponentInfo *p = info->parent; p; p = p->parent) chain.push_back(p);
            for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
                runConstructionBody(*it, inst, *out, cgi, server, cookie, application, session, url, form);
            }
        }
        runConstructionBody(info, inst, *out, cgi, server, cookie, application, session, url, form);
        if (inst->thisScope && inst->thisScope->has("MAPPINGS")) {
            cfml::app_mappings_set(&(*inst->thisScope)["MAPPINGS"]);
        }
    } catch (...) {
        component_instance_release(inst);
        throw;
    }

    return makeComponentVariant(inst, false);
}

// Builds a Component cfvariant from a live instance, sharing the this scope's
// StructData so struct introspection / SerializeJSON / member access work.
// When `retain` is true the variant takes its own reference to the instance
// (for temporary `this` values that share an instance owned elsewhere);
// cf_component_instantiate passes false because it transfers its ownership.
cfvariant *makeComponentVariant(ComponentInstance *inst, bool retain)
{
    auto *comp = new cfvariant(cfvariant::Component);
    comp->m_component = retain ? component_instance_retain(inst) : inst;
    struct_data_release(comp->m_structData);  // drop the empty one created by set_type
    comp->m_structData = struct_data_retain(inst->thisScope->m_structData);
    comp->m_struct = &comp->m_structData->map;
    comp->m_structInsertOrder = &comp->m_structData->insertOrder;
    cf_register_temp(comp);
    return comp;
}

// The `this` value inside a component method/body: a temporary Component
// variant wrapping the current instance (CF's `this` is the object itself).
cfvariant *componentThisValue(ComponentInstance *inst)
{
    if (!inst) return nullptr;
    return makeComponentVariant(inst, true);
}

// ---- Method invocation ----

// Invoke a method JIT entry with the instance's scopes and the standard
// component-method call-context bookkeeping (mirrors cf_udf_invoke).
static cfvariant *invokeMethodEntry(ComponentInstance *inst, ComponentInfo *ownerInfo, ComponentMethod &m,
                                    const cfvariant **args, int argc,
                                    string &out, void *cgi, void *server, void *cookie,
                                    void *application, void *session, void *url, void *form)
{
    if (!m.fn) {
        throw webstrada::exception("component",
            webstrada::string(("Method " + m.name + " has no compiled body.").c_str()));
    }
    auto entry = reinterpret_cast<component_method_entry_fn>(m.fn);
    // Reorder named arguments (marker at args[0]) against the method's declared
    // parameter names so the JIT prologue binds positionally.
    std::vector<const cfvariant*> reordered;
    int effectiveArgc = argc;
    const cfvariant **effectiveArgs = args;
    {
        std::vector<const char*> names;
        for (const auto &p : m.paramNames) names.push_back(p.c_str());
        if (cf_named_args_reorder(args, argc, names.empty() ? nullptr : names.data(),
                                  static_cast<int>(names.size()), reordered, effectiveArgc)) {
            effectiveArgs = reordered.data();
        }
    }
    size_t ctxSave = g_udfCtx.size();
    g_currentMethodOwnerInfo = ownerInfo;

    IncludeRuntime *rt = cfml::include_context();
    std::string prevPath;
    if (rt) {
        prevPath = rt->currentPath;
        if (ownerInfo && !ownerInfo->cfcPath.empty()) {
            rt->currentPath = ownerInfo->cfcPath;
        } else if (inst && inst->info && !inst->info->cfcPath.empty()) {
            rt->currentPath = inst->info->cfcPath;
        }
    }

    try {
        cfvariant *res = entry(&out, cgi, server, cookie, application, session, url, form,
                               inst->variablesScope, inst->thisScope, inst, effectiveArgs, effectiveArgc);
        if (rt) rt->currentPath = prevPath;
        while (g_udfCtx.size() > ctxSave) g_udfCtx.pop_back();
        return res;
    } catch (...) {
        if (rt) rt->currentPath = prevPath;
        while (g_udfCtx.size() > ctxSave) g_udfCtx.pop_back();
        throw;
    }
}

// Find the method by name on a component value and invoke it. `enforceAccess`
// is true for external entry points (member dispatch / cfinvoke / CreateObject
// method call); internal calls (a method calling another method unqualified)
// pass false so private methods resolve. Returns a temp.
static cfvariant *invokeComponentValue(cfvariant *compVal, const std::string &methodName,
                                       const cfvariant **args, int argc,
                                       string &out, void *cgi, void *server, void *cookie,
                                       void *application, void *session, void *url, void *form,
                                       bool enforceAccess)
{
    if (!compVal || compVal->m_type != cfvariant::Component || !compVal->m_component) {
        throw webstrada::exception("Entity has incorrect type for being called as a function.");
    }
    ComponentInstance *inst = compVal->m_component;
    std::string upper = methodName;
    for (auto &c : upper) c = (char)toupper((unsigned char)c);

    ComponentInfo *startInfo = compVal->m_superTargetInfo ? compVal->m_superTargetInfo : inst->info;
    ComponentInfo *owner = nullptr;
    int idx = findMethodInInfo(startInfo, upper, owner);
    if (idx < 0) {
        cf_component_throw_method_not_found(compVal, methodName.c_str());
    }
    ComponentMethod &m = owner->methods[idx];
    if (enforceAccess && !compVal->m_superTargetInfo && !methodExternallyCallable(m)) {
        cf_component_throw_method_not_found(compVal, methodName.c_str());
    }
    return invokeMethodEntry(inst, owner, m, args, argc, out, cgi, server, cookie, application, session, url, form);
}

cfvariant *cf_component_invoke(cfvariant *compVal, const char *methodName,
                               const cfvariant **args, int argc,
                               string &out, void *cgi, void *server, void *cookie,
                               void *application, void *session, void *url, void *form)
{
    return invokeComponentValue(compVal, methodName, args, argc, out, cgi, server, cookie,
                                application, session, url, form, true);
}

cfvariant *cf_component_invoke_instance(ComponentInstance *inst, const char *methodName,
                                        const cfvariant **args, int argc,
                                        string &out, void *cgi, void *server, void *cookie,
                                        void *application, void *session, void *url, void *form)
{
    if (!inst) throw webstrada::exception("component", "Attempt to invoke a method on an undefined component.");
    std::string upper = methodName;
    for (auto &c : upper) c = (char)toupper((unsigned char)c);
    ComponentInfo *owner = nullptr;
    int idx = findMethodInInstance(inst, upper, owner);
    if (idx < 0) {
        throw webstrada::exception("component",
            webstrada::string(("Method " + upper + " was not found in the component.").c_str()));
    }
    return invokeMethodEntry(inst, owner, owner->methods[idx], args, argc, out, cgi, server, cookie,
                             application, session, url, form);
}

cfvariant *cf_component_method_handle_invoke(cfvariant *handleVal,
                                             const cfvariant **args, int argc,
                                             string &out, void *cgi, void *server, void *cookie,
                                             void *application, void *session, void *url, void *form)
{
    if (!handleVal || handleVal->m_type != cfvariant::Function || !handleVal->m_udf ||
        handleVal->m_udf->componentMethodIndex < 0 || !handleVal->m_udf->component) {
        throw webstrada::exception("Entity has incorrect type for being called as a function.");
    }
    UDFInfo *info = handleVal->m_udf;
    ComponentInstance *inst = info->component;
    std::string upper(info->name.constData() ? info->name.constData() : "");
    for (auto &c : upper) c = (char)toupper((unsigned char)c);
    ComponentInfo *startInfo = handleVal->m_superTargetInfo ? handleVal->m_superTargetInfo : inst->info;
    ComponentInfo *owner = nullptr;
    int idx = findMethodInInfo(startInfo, upper, owner);
    if (idx < 0) {
        throw webstrada::exception("component",
            webstrada::string(("Method " + upper + " was not found.").c_str()));
    }
    return invokeMethodEntry(inst, owner, owner->methods[idx], args, argc, out, cgi, server, cookie,
                             application, session, url, form);
}

cfvariant *cf_component_method_handle(cfvariant *compVal, int methodIndex)
{
    if (!compVal || compVal->m_type != cfvariant::Component || !compVal->m_component) {
        throw webstrada::exception("Cannot access a method on a non-component value.");
    }
    ComponentInstance *inst = compVal->m_component;
    // The method index is flattened across the extends chain (parent methods
    // first). Map it back to the owning ComponentInfo.
    ComponentInfo *owner = nullptr;
    int idx = methodIndex;
    for (ComponentInfo *i = inst->info; i; i = i->parent) {
        if (idx < static_cast<int>(i->methods.size())) { owner = i; break; }
        idx -= static_cast<int>(i->methods.size());
    }
    if (!owner || idx < 0) {
        throw webstrada::exception("component", "Method index out of range.");
    }
    ComponentMethod &m = owner->methods[idx];
    auto *ui = new UDFInfo();
    ui->fn = m.fn;
    ui->name = webstrada::string(m.name.c_str());
    ui->access = webstrada::string(m.access.c_str());
    ui->returnType = webstrada::string(m.returnType.c_str());
    ui->params = m.params;
    ui->componentMethodIndex = methodIndex;
    ui->component = component_instance_retain(inst);
    auto *ret = new cfvariant(cfvariant::Function);
    *ret->m_str = ui->name;
    ret->m_udf = ui;
    cf_register_temp(ret);
    return ret;
}

int cf_component_has_method(const cfvariant *compVal, const char *methodName)
{
    if (!compVal || compVal->m_type != cfvariant::Component || !compVal->m_component) return 0;
    std::string upper = methodName;
    for (auto &c : upper) c = (char)toupper((unsigned char)c);
    ComponentInfo *owner = nullptr;
    int idx = findMethodInInstance(compVal->m_component, upper, owner);
    if (idx < 0) return 0;
    return methodExternallyCallable(owner->methods[idx]) ? 1 : 0;
}

int cf_component_has_method_on(ComponentInstance *inst, const char *methodName)
{
    if (!inst || !inst->info) return 0;
    std::string upper = methodName;
    for (auto &c : upper) c = (char)toupper((unsigned char)c);
    ComponentInfo *owner = nullptr;
    return findMethodInInstance(inst, upper, owner) >= 0 ? 1 : 0;
}

// Component member access: a data member of the this scope wins, otherwise an
// externally callable method is exposed as a callable method-handle Function
// value (temp). Returns nullptr when the member does not exist.
void cf_component_append_method_keys(const cfvariant *compVal, std::vector<webstrada::string> &keys)
{
    if (!compVal || compVal->m_type != cfvariant::Component || !compVal->m_component) return;
    ComponentInstance *inst = compVal->m_component;
    std::vector<ComponentInfo*> chain;
    for (ComponentInfo *i = inst->info; i; i = i->parent) chain.push_back(i);
    // Declaration order: parent methods first, then child. Method names keep
    // their declared casing (CF lists them that way in StructKeyList).
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        for (const auto &m : (*it)->methods) {
            if (methodExternallyCallable(m)) {
                keys.push_back(webstrada::string(m.declaredName.empty() ? m.name.c_str() : m.declaredName.c_str()));
            }
        }
    }
}

void cf_component_throw_method_not_found(const cfvariant *compVal, const char *methodName)
{
    std::string compName = "unknown";
    if (compVal && compVal->m_type == cfvariant::Component && compVal->m_component &&
        compVal->m_component->info) {
        compName = compVal->m_component->info->name;
    }
    std::string msg = "Neither the method " + std::string(methodName) +
                      " was found in component " + compName +
                      " nor was there any default method with this name present in any of the implementing interface.";
    throw webstrada::exception("Application", webstrada::string(msg.c_str()),
                              webstrada::string(("The specific sequence of files included or processed is: " + compName).c_str()));
}

// cfscript `new path(args)`: load + instantiate + auto-call init(args).
cfvariant *cf_component_new(const char *path, const cfvariant **args, int argc,
                            string &out, void *cgi, void *server, void *cookie,
                            void *application, void *session, void *url, void *form,
                            void *variables)
{
    ComponentInfo *info = cf_component_load(path);
    if (!info) {
        throw webstrada::exception("component",
            webstrada::string(("The component " + std::string(path) + " could not be found.").c_str()));
    }
    struct NewInfoGuard {
        ComponentInfo *i;
        ~NewInfoGuard() { if (i) component_info_release(i); }
    } newInfoGuard{info};
    cfvariant *comp = cf_component_instantiate(info, static_cast<cfvariant*>(variables),
                                               &out, cgi, server, cookie, application, session, url, form);
    newInfoGuard.i = nullptr;
    component_info_release(info);  // drop the loader retain (cache still holds its ref)
    // `new` auto-calls init with the constructor arguments (CF behavior).
    if (cf_component_has_method(comp, "INIT")) {
        std::vector<const cfvariant*> argPtrs;
        for (int i = 0; i < argc; i++) argPtrs.push_back(args[i]);
        cfvariant *initRet = cf_component_invoke(comp, "init", argPtrs.data(), argc,
                                                 out, cgi, server, cookie, application, session, url, form);
        (void)initRet;
    }
    return comp;
}

// <cfobject type="component">: instantiate the component and store it in the
// `name` variable (cfvariant_assign). Throws for unsupported object types.
void cf_cfobject(string *out, void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables,
                 const cfvariant *type, const cfvariant *name, const cfvariant *component)
{
    if (!type || !name || !component) {
        throw webstrada::exception("cfobject", "The type, name and component attributes are required.");
    }
    string typeStr = const_cast<cfvariant*>(type)->toString();
    string t = typeStr;
    t.toLower();
    if (!t.equals("component")) {
        throw webstrada::exception("cfobject",
            webstrada::string(("The object type " + std::string(typeStr.constData() ? typeStr.constData() : "") + " is not supported.").c_str()));
    }
    string path = const_cast<cfvariant*>(component)->toString();
    ComponentInfo *info = cf_component_load(path.constData());
    if (!info) {
        throw webstrada::exception("cfobject",
            webstrada::string(("The component " + std::string(path.constData() ? path.constData() : "") + " could not be found.").c_str()));
    }
    cfvariant *inst = cf_component_instantiate(info, static_cast<cfvariant*>(variables),
                                               out, cgi, server, cookie, application, session, url, form);
    component_info_release(info);
    string varName = const_cast<cfvariant*>(name)->toString();
    cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                     static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                     static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                     static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                     varName.constData(), inst);
}

// ---- <cfinvoke> / <cfinvokeargument> ----

// <cfinvoke> start tag: push a call context carrying the (runtime-evaluated)
// component/method/returnvariable/argumentcollection attribute values; the
// body's <cfinvokeargument> children append named arguments; cf_cfinvoke_end
// performs the invoke and assigns the returnvariable.
void cf_cfinvoke_begin(const cfvariant *component, const cfvariant *method,
                       const cfvariant *returnvariable, const cfvariant *argumentcollection)
{
    auto *ctx = new InvokeCtx();
    ctx->component = component;
    ctx->method = method;
    ctx->returnvariable = returnvariable;
    ctx->argumentcollection = argumentcollection;
    g_invokeCtxs.push_back(ctx);
}

// <cfinvokeargument>: append a named argument to the innermost <cfinvoke>.
void cf_cfinvoke_argument(const cfvariant *name, const cfvariant *value)
{
    if (g_invokeCtxs.empty()) {
        throw webstrada::exception("cfinvokeargument is only valid inside a cfinvoke tag.");
    }
    InvokeArg a;
    if (name) a.name = const_cast<cfvariant*>(name)->toString();
    a.value = value;
    g_invokeCtxs.back()->args.push_back(std::move(a));
}

namespace {

// Builds a named-arguments marker struct for the invocation, merged from the
// argumentcollection and the <cfinvokeargument> children (later entries win,
// like CF's AttributeCollection). `paramNames` (nullable) is the target
// method's declared parameter names: entries that do not match any parameter
// are dropped, matching CF's _invoke (which silently ignores unknown named
// arguments). Returns a stack marker that must outlive the invoke.
bool buildInvokeNamedArgs(const InvokeCtx *ctx, const std::vector<std::string> *paramNames,
                          cfvariant &namedOut, cfvariant &markerOut,
                          std::vector<const cfvariant*> &argPtrs)
{
    (void)namedOut;
    cfvariant named(cfvariant::Struct);
    if (ctx->argumentcollection && ctx->argumentcollection->m_type == cfvariant::Struct &&
        ctx->argumentcollection->m_struct) {
        for (const auto &kv : *ctx->argumentcollection->m_struct) {
            named.structSet(kv.first, kv.second);
        }
    }
    for (const auto &a : ctx->args) {
        if (a.value) named.structSet(a.name, *a.value);
    }
    if (named.m_struct && !named.m_struct->empty()) {
        cfvariant filtered(cfvariant::Struct);
        if (paramNames) {
            for (const auto &kv : *named.m_struct) {
                bool match = false;
                for (const auto &pn : *paramNames) {
                    webstrada::string pnS(pn.c_str());
                    if (pnS.compareCaseInsensitive(kv.first) == 0) { match = true; break; }
                }
                if (match) filtered.structSet(kv.first, kv.second);
            }
        } else {
            for (const auto &kv : *named.m_struct) filtered.structSet(kv.first, kv.second);
        }
        // The named-arguments marker (a Struct holding the named struct under
        // CFML_NAMED_ARGS_KEY), built on the stack like cf_named_args_marker so
        // the invoke does not leak a heap marker per call.
        markerOut = cfvariant(cfvariant::Struct);
        markerOut.structSet(CFML_NAMED_ARGS_KEY, filtered);
        argPtrs.push_back(&markerOut);
        return true;
    }
    return false;
}

} // namespace

// <cfinvoke> end tag: pop the context, resolve the target (a component value,
// a component path string, or — when `component` is absent — a UDF in the page
// variables scope, like CF's InvokeTag) and invoke it with the merged named
// arguments. Assigns the result to `returnvariable` when present. Returns the
// result (a temp owned by the caller's temp registry).
cfvariant *cf_cfinvoke_end(string *out, void *cgi, void *server, void *cookie, void *application,
                           void *session, void *url, void *form, void *variables)
{
    if (g_invokeCtxs.empty()) {
        throw webstrada::exception("cfinvoke", "No active cfinvoke context.");
    }
    InvokeCtx *ctx = g_invokeCtxs.back();
    g_invokeCtxs.pop_back();
    std::unique_ptr<InvokeCtx> guard(ctx);

    if (!ctx->method) {
        throw webstrada::exception("cfinvoke", "The method attribute is required.");
    }
    std::string methodName = safe_to_std_string(*ctx->method);

    ComponentInfo *loadedInfo = nullptr;
    cfvariant *comp = nullptr;
    cfvariant loadedComp;
    if (ctx->component && ctx->component->m_type == cfvariant::Component) {
        comp = const_cast<cfvariant*>(ctx->component);
    } else if (ctx->component) {
        string path = const_cast<cfvariant*>(ctx->component)->toString();
        loadedInfo = cf_component_load(path.constData());
        if (!loadedInfo) {
            throw webstrada::exception("cfinvoke",
                webstrada::string(("The component " + std::string(path.constData() ? path.constData() : "") + " could not be found.").c_str()));
        }
        loadedComp = *cf_component_instantiate(loadedInfo, static_cast<cfvariant*>(variables),
                                               out, cgi, server, cookie, application, session, url, form);
        comp = &loadedComp;
    }

    cfvariant *res = nullptr;
    cfvariant namedOut, markerOut;
    std::vector<const cfvariant*> argPtrs;
    std::vector<std::string> paramNames;

    if (comp) {
        ComponentInstance *inst = comp->m_component;
        std::string upper = methodName;
        for (auto &c : upper) c = (char)toupper((unsigned char)c);
        ComponentInfo *owner = nullptr;
        int idx = findMethodInInfo(inst->info, upper, owner);
        if (idx < 0) {
            if (loadedInfo) cf_component_info_release(loadedInfo);
            cf_component_throw_method_not_found(comp, methodName.c_str());
        }
        if (owner) {
            for (const auto &pn : owner->methods[idx].paramNames) paramNames.push_back(pn);
        }
        if (buildInvokeNamedArgs(ctx, owner ? &paramNames : nullptr, namedOut, markerOut, argPtrs)) {
            res = cf_component_invoke(comp, methodName.c_str(), argPtrs.data(),
                                      static_cast<int>(argPtrs.size()),
                                      *out, cgi, server, cookie, application, session, url, form);
        } else {
            res = cf_component_invoke(comp, methodName.c_str(), nullptr, 0,
                                      *out, cgi, server, cookie, application, session, url, form);
        }
    } else {
        // No component: invoke a UDF by name in the page's variables scope.
        cfvariant *udfVal = lookupVarWritable(methodName.c_str(), cgi, server, cookie, application,
                                              session, url, form, variables);
        if (!udfVal || udfVal->m_type != cfvariant::Function || !udfVal->m_udf || !udfVal->m_udf->fn) {
            throw webstrada::exception("Entity has incorrect type for being called as a function.");
        }
        for (const auto &p : udfVal->m_udf->params) paramNames.push_back(p.name.constData() ? p.name.constData() : "");
        if (buildInvokeNamedArgs(ctx, &paramNames, namedOut, markerOut, argPtrs)) {
            res = cf_udf_invoke(udfVal, argPtrs.data(), static_cast<int>(argPtrs.size()),
                                *out, cgi, server, cookie, application, session, url, form, variables);
        } else {
            res = cf_udf_invoke(udfVal, nullptr, 0,
                                *out, cgi, server, cookie, application, session, url, form, variables);
        }
    }

    if (ctx->returnvariable && res) {
        string varName = const_cast<cfvariant*>(ctx->returnvariable)->toString();
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         varName.constData(), res);
    }
    if (loadedInfo) cf_component_info_release(loadedInfo);
    return res;
}

// <cfinvoke> single-shot form (used by the self-closing <cfinvoke/> codegen
// path; delegates to the begin/argument/end context so the behavior is shared).
cfvariant *cf_cfinvoke(string *out, void *cgi, void *server, void *cookie, void *application,
                       void *session, void *url, void *form, void *variables,
                       const cfvariant *component, const cfvariant *method,
                       const cfvariant *returnvariable, const cfvariant *argumentcollection)
{
    cf_cfinvoke_begin(component, method, returnvariable, argumentcollection);
    return cf_cfinvoke_end(out, cgi, server, cookie, application, session, url, form, variables);
}

// <cfimport path="...">: register a component import path (a dotted path or a
// "prefix.*" wildcard). CreateObject/`new` fall back to these when the plain
// relative resolution misses (CF's importList).
void cf_import_path(const cfvariant *path)
{
    if (!path) return;
    string p = const_cast<cfvariant*>(path)->toString();
    if (p.constData() && p.constData()[0]) g_importPaths.push_back(p.constData());
}

// <cfimport taglib/prefix> and <cfmodule>/<cfassociate> need the custom-tag
// runtime, which this engine does not implement. They throw a catchable
// Application error (per AGENTS.md, an unimplementable construct throws a
// runtime exception with a corresponding message).
void cf_import_taglib(const cfvariant *taglib, const cfvariant *prefix)
{
    (void)taglib;
    (void)prefix;
    throw webstrada::exception("Application", "cfimport with a taglib/prefix is not supported.",
        "Custom tags are not implemented in this engine.");
}

void cf_cfmodule(const cfvariant *templateAttr, const cfvariant *nameAttr,
                 const cfvariant *attributecollection)
{
    (void)templateAttr;
    (void)nameAttr;
    (void)attributecollection;
    throw webstrada::exception("Application", "cfmodule is not supported.",
        "Custom tags are not implemented in this engine.");
}

void cf_cfassociate(const cfvariant *basetag, const cfvariant *datacollection)
{
    (void)basetag;
    (void)datacollection;
    throw webstrada::exception("Application", "cfassociate is not supported.",
        "Custom tags are not implemented in this engine.");
}

// GetComponentMetaData(obj): the component's introspection struct (name, path,
// type, functions, properties, extends...). Built from the ComponentInfo.
cfvariant *cf_getcomponentmetadata_impl(const cfvariant *compVal)
{
    if (!compVal || compVal->m_type != cfvariant::Component || !compVal->m_component ||
        !compVal->m_component->info) {
        throw webstrada::exception("GetComponentMetaData", "The value is not a component.");
    }
    ComponentInstance *inst = compVal->m_component;
    ComponentInfo *info = inst->info;
    cfvariant *md = new cfvariant(cfvariant::Struct);
    md->structSet("name", cfvariant(info->name.c_str()));
    md->structSet("path", cfvariant(info->name.c_str()));
    md->structSet("type", cfvariant("component"));
    md->structSet("fullname", cfvariant(info->name.c_str()));

    cfvariant funcs(cfvariant::Array);
    for (const auto &m : info->methods) {
        cfvariant f(cfvariant::Struct);
        f.structSet("name", cfvariant(m.declaredName.empty() ? m.name.c_str() : m.declaredName.c_str()));
        f.structSet("access", cfvariant(m.access.c_str()));
        f.structSet("returntype", cfvariant(m.returnType.c_str()));
        f.structSet("output", cfvariant("true"));
        cfvariant params(cfvariant::Array);
        for (const auto &p : m.params) {
            cfvariant prm(cfvariant::Struct);
            prm.structSet("name", p.name);
            prm.structSet("type", p.type);
            prm.structSet("required", cfvariant(cfvariant::Boolean));
            prm.structSet("default", p.defaultValue);
            params.insert(prm);
        }
        f.structSet("parameters", params);
        funcs.insert(f);
    }
    md->structSet("functions", funcs);

    cfvariant props(cfvariant::Array);
    for (const auto &p : info->properties) {
        cfvariant pr(cfvariant::Struct);
        pr.structSet("name", cfvariant(p.name.c_str()));
        pr.structSet("type", cfvariant(p.type.c_str()));
        pr.structSet("default", cfvariant(p.defaultText.c_str()));
        pr.structSet("access", cfvariant(p.access.c_str()));
        props.insert(pr);
    }
    md->structSet("properties", props);

    if (info->parent) {
        md->structSet("extends", cfvariant(info->parent->name.c_str()));
    }
    cf_register_temp(md);
    return md;
}

// IsInstanceOf(obj, typeName): true when obj is a component of (or extending)
// the named type (case-insensitive, dot paths normalized).
cfvariant *cf_isinstanceof_impl(const cfvariant *obj, const cfvariant *typeName)
{
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    if (!obj || !typeName || obj->m_type != cfvariant::Component || !obj->m_component ||
        !obj->m_component->info) {
        return ret;
    }
    std::string wanted = const_cast<cfvariant*>(typeName)->toString().constData()
        ? std::string(const_cast<cfvariant*>(typeName)->toString().constData()) : "";
    std::string up = wanted;
    for (auto &c : up) c = (char)toupper((unsigned char)c);
    std::string lastUp = up;
    size_t lastDot = wanted.find_last_of('.');
    if (lastDot != std::string::npos) lastUp = up.substr(lastDot + 1);

    // Whether an interface (or any of its extends parents) matches the name.
    std::function<bool(ComponentInfo*)> ifaceMatches = [&](ComponentInfo *iface) -> bool {
        std::string full = iface->fullName.empty() ? iface->name : iface->fullName;
        std::string fu = full;
        for (auto &c : fu) c = (char)toupper((unsigned char)c);
        if (fu == up) return true;
        size_t d = full.find_last_of('.');
        std::string seg = (d == std::string::npos) ? full : full.substr(d + 1);
        for (auto &c : seg) c = (char)toupper((unsigned char)c);
        if (seg == lastUp) return true;
        for (ComponentInfo *p : iface->interfaceParents) {
            if (ifaceMatches(p)) return true;
        }
        return false;
    };

    // Walk the extends chain: each component's own name/path plus the
    // interfaces it implements (directly or inherited).
    for (ComponentInfo *i = obj->m_component->info; i; i = i->parent) {
        std::string full = i->fullName.empty() ? i->name : i->fullName;
        std::string fu = full;
        for (auto &c : fu) c = (char)toupper((unsigned char)c);
        if (fu == up) { ret->m_bool = true; break; }
        size_t d = full.find_last_of('.');
        std::string seg = (d == std::string::npos) ? full : full.substr(d + 1);
        for (auto &c : seg) c = (char)toupper((unsigned char)c);
        if (seg == lastUp) { ret->m_bool = true; break; }
        for (ComponentInfo *iface : i->interfaces) {
            if (ifaceMatches(iface)) { ret->m_bool = true; break; }
        }
        if (ret->m_bool) break;
    }
    return ret;
}

} // namespace cfml

namespace webstrada {

// Descend a dotted member path (e.g. "A.B.C") from `base` following the scope
// lookup rules: query columns, struct/xml members and component members
// (this-scope data or method handles). Returns nullptr when any segment fails.
cfvariant *descendDottedPath(cfvariant *base, const std::vector<webstrada::string> &parts,
                             size_t startIdx)
{
    cfvariant *current = base;
    for (size_t i = startIdx; i < parts.size(); i++) {
        if (current && current->m_type == cfvariant::Query && current->m_query) {
            current = resolveQueryMember(current, parts[i].constData());
            if (!current) return nullptr;
            continue;
        }
        if (current && current->m_type == cfvariant::Component && current->m_component) {
            // Component: this-scope data member or a method handle.
            if (current->m_struct) {
                auto it = current->m_struct->find(parts[i]);
                if (it != current->m_struct->end()) {
                    current = &it->second;
                    continue;
                }
            }
            current = componentMemberAccess(current, parts[i]);
            if (!current) return nullptr;
            continue;
        }
        if (!current || (current->m_type != cfvariant::Struct && current->m_type != cfvariant::Xml) || current->m_disabled) {
            return nullptr;
        }
        auto it = current->m_struct->find(parts[i]);
        if (it == current->m_struct->end()) {
            if (current->m_type == cfvariant::Xml && current->m_struct) {
                auto itChildren = current->m_struct->find("XMLCHILDREN");
                if (itChildren != current->m_struct->end() && itChildren->second.m_type == cfvariant::Array && itChildren->second.m_array) {
                    std::vector<cfvariant> matches;
                    for (auto &child : *itChildren->second.m_array) {
                        if (child.m_type == cfvariant::Xml && child.m_struct) {
                            auto itName = child.m_struct->find("XMLNAME");
                            if (itName != child.m_struct->end() && itName->second.toString().compareCaseInsensitive(parts[i]) == 0) {
                                matches.push_back(child);
                            }
                        }
                    }
                    if (matches.size() == 1) {
                        (*current->m_struct)[parts[i]] = matches[0];
                        current = &(*current->m_struct)[parts[i]];
                        continue;
                    } else if (matches.size() > 1) {
                        cfvariant childGroup(current->m_upcase, false);
                        childGroup.set_type(cfvariant::Array);
                        childGroup.m_isXmlNodeList = true;
                        for (auto const& ch : matches) childGroup.insert(ch);
                        (*current->m_struct)[parts[i]] = childGroup;
                        current = &(*current->m_struct)[parts[i]];
                        continue;
                    }
                }
            }
            return nullptr;
        }
        current = &it->second;
    }
    return current;
}

// Component member access: this-scope data member or a callable method-handle
// for an externally callable method, or nullptr when the member does not exist.
cfvariant *componentMemberAccess(cfvariant *comp, const webstrada::string &key)
{
    if (!comp || comp->m_type != cfvariant::Component || !comp->m_component) return nullptr;
    std::string upper(key.constData() ? key.constData() : "");
    for (auto &c : upper) c = (char)toupper((unsigned char)c);
    ComponentInstance *inst = comp->m_component;
    ComponentInfo *startInfo = comp->m_superTargetInfo ? comp->m_superTargetInfo : inst->info;
    ComponentInfo *owner = nullptr;
    int idx = findMethodInInfo(startInfo, upper, owner);
    if (idx < 0) return nullptr;
    ComponentMethod &m = owner->methods[idx];
    if (!comp->m_superTargetInfo && !methodExternallyCallable(m)) return nullptr;
    // Flatten the method index child-first (matching cf_component_method_handle).
    int pos = 0;
    int flat = -1;
    for (ComponentInfo *ci = inst->info; ci; ci = ci->parent) {
        for (size_t j = 0; j < ci->methods.size(); j++) {
            if (ci == owner && ci->methods[j].name == m.name) { flat = pos; break; }
            pos++;
        }
        if (flat >= 0) break;
    }
    if (flat < 0) return nullptr;
    cfvariant *h = cfml::cf_component_method_handle(comp, flat);
    if (comp->m_superTargetInfo && h) {
        h->m_superTargetInfo = comp->m_superTargetInfo;
    }
    return h;
}

} // namespace webstrada

namespace cfml {

cfvariant *cf_component_get_super_scope()
{
    for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
        if (it->component && it->component->info) {
            ComponentInfo *targetInfo = it->componentInfo ? it->componentInfo->parent : it->component->info->parent;
            if (!targetInfo) {
                throw webstrada::exception("component", "Component has no parent component (extends nothing).");
            }
            cfvariant *superVal = makeComponentVariant(it->component, true);
            superVal->m_superTargetInfo = targetInfo;
            return superVal;
        }
    }
    throw webstrada::exception("component", "SUPER scope is only available inside a component method.");
}

} // namespace cfml

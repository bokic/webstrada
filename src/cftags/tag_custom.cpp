/**
 * @file tag_custom.cpp
 * @brief Custom tag runtime implementation (cf_custom_tag_*).
 */

#include "common.h"
#include "../core/core_internal.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

using webstrada::string;
using webstrada::cfvariant;

bool resolveCustomTagPath(const std::string &tpl, const cfml::IncludeRuntime *rt,
                          std::string &resolved)
{
    std::string normalizedTpl = tpl;
    for (auto &c : normalizedTpl) {
        if (c == '\\') c = '/';
    }
    if (cfml::app_mappings_resolve(normalizedTpl, resolved)) {
        return true;
    }

    std::filesystem::path base;
    if (!normalizedTpl.empty() && normalizedTpl[0] == '/') {
        if (rt->webRoot.empty()) return false;
        base = std::filesystem::path(rt->webRoot);
    } else {
        if (rt->currentPath.empty()) return false;
        std::filesystem::path cur(rt->currentPath);
        base = cur.has_parent_path() ? cur.parent_path() : std::filesystem::path("");
    }
    if (normalizedTpl.empty()) return false;
    std::filesystem::path input(normalizedTpl);
    if (input.is_absolute()) {
        input = input.relative_path();
    }
    std::filesystem::path full = base / input;
    resolved = full.lexically_normal().string();
    return true;
}

bool customTagFileExists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

} // namespace

namespace cfml {

void cf_custom_tag_begin(const char *tagName, cfvariant *attrs, bool hasEndTag, cfvariant *callerVariables,
                         bool isModule, const char *templateNameHint)
{
    CustomTagCallCtx ctx;
    std::string base = tagName ? tagName : "";
    for (auto &c : base) c = toupper((unsigned char)c);

    // Public name used by GetBaseTagList: CF_<NAME>.
    ctx.publicName = "CF_" + base;
    // Template name used by GetBaseTagData matching: a prefixed/imported tag
    // matches "cf_<name>"; a cfmodule matches "CF_<name>".
    if (isModule) {
        ctx.templateName = ctx.publicName;
    } else {
        ctx.templateName = "cf_";
        for (auto &c : base) c = tolower((unsigned char)c);
        ctx.templateName += base;
    }
    if (templateNameHint) {
        std::string hint = templateNameHint;
        if (isModule) {
            // cfmodule template="path/name.cfm" -> public name CF_NAME.
            size_t slash = hint.find_last_of('/');
            if (slash != std::string::npos) hint = hint.substr(slash + 1);
            size_t dot = hint.find_last_of('.');
            if (dot != std::string::npos) hint = hint.substr(0, dot);
            for (auto &c : hint) c = toupper((unsigned char)c);
            ctx.publicName = "CF_" + hint;
            ctx.templateName = ctx.publicName;
        } else {
            for (auto &c : hint) c = tolower((unsigned char)c);
            ctx.templateName = "cf_" + hint;
        }
    }
    ctx.tagName = base;

    // Initialize attributes struct
    if (attrs && attrs->m_type == cfvariant::Struct) {
        ctx.attributes = *attrs;
    } else {
        ctx.attributes.set_type(cfvariant::Struct);
    }

    // Initialize variables struct (private to this custom tag invocation)
    ctx.variables.set_type(cfvariant::Struct);

    ctx.callerVariables = callerVariables;
    ctx.hasEndTag = hasEndTag;
    ctx.loopRequested = false;
    ctx.skipBody = false;
    ctx.contentChanged = false;

    // Initialize thisTag struct. CF's ThistagScope stores the keys with this
    // exact casing ("executionMode", "hasendtag", "GeneratedContent"); the
    // scope is a plain case-insensitive struct, so lookups work regardless.
    ctx.thisTag.set_type(cfvariant::Struct);

    ctx.thisTag.set("executionMode") = cfvariant(string("start"));
    ctx.thisTag.set("hasendtag") = cfvariant(string(hasEndTag ? "YES" : "NO"));
    ctx.thisTag.set("GeneratedContent") = cfvariant(string(""));

    g_customTagStack.push_back(std::move(ctx));
}

void cf_custom_tag_end_mode(const string *generatedContent)
{
    if (g_customTagStack.empty()) return;
    CustomTagCallCtx &ctx = g_customTagStack.back();
    ctx.thisTag.set("executionMode") = cfvariant(string("end"));
    if (generatedContent) {
        ctx.thisTag.set("GeneratedContent") = cfvariant(string(generatedContent->constData(), generatedContent->length()));
    } else {
        ctx.thisTag.set("GeneratedContent") = cfvariant(string(""));
    }
    ctx.loopRequested = false;
    ctx.contentChanged = false;
}

void cf_custom_tag_finish()
{
    if (!g_customTagStack.empty()) {
        g_customTagStack.pop_back();
    }
}

bool cf_custom_tag_should_loop()
{
    if (g_customTagStack.empty()) return false;
    return g_customTagStack.back().loopRequested;
}

// A bare `<cfexit>` / method="exittag" executed from the custom tag's START
// template makes ColdFusion skip the body and the end tag entirely.
bool cf_custom_tag_should_skip_body()
{
    if (g_customTagStack.empty()) return false;
    return g_customTagStack.back().skipBody;
}

// The current custom tag's thisTag.generatedContent was assigned by CFML;
// ColdFusion then replaces the captured body with the new generated content.
void cf_custom_tag_mark_content_changed()
{
    if (g_customTagStack.empty()) return;
    g_customTagStack.back().contentChanged = true;
}

// <cfmodule> tag template path: the `template` value as-is, or `name` + ".cfm".
cfvariant *cf_custom_tag_module_path(const cfvariant *templateAttr, const cfvariant *nameAttr)
{
    if (templateAttr && templateAttr->m_type != cfvariant::Null) {
        const string p = variantToString(*templateAttr);
        auto *ret = new cfvariant(p);
        cf_register_temp(ret);
        return ret;
    }
    if (nameAttr && nameAttr->m_type != cfvariant::Null) {
        const string n = variantToString(*nameAttr);
        webstrada::string p = n;
        p += ".cfm";
        auto *ret = new cfvariant(p);
        cf_register_temp(ret);
        return ret;
    }
    throw webstrada::exception("cfmodule", "The TEMPLATE or NAME attribute is required.");
}

// Merges a <cfmodule> attributecollection into the built attributes struct.
// Explicit attributes win, so collection entries are only added when absent.
void cf_custom_tag_merge_attributecollection(cfvariant *attrs, const cfvariant *collection)
{
    if (!attrs || !collection || collection->m_type != cfvariant::Struct) return;
    for (const auto &pair : *collection->m_struct) {
        if (!attrs->has(pair.first.constData())) {
            attrs->set(pair.first.constData()) = pair.second;
        }
    }
}

void cf_custom_tag_invoke(string *out, void *cgi, void *server, void *cookie, void *application,
                          void *session, void *url, void *form, void *variables,
                          cfvariant *tagPathVar, const char *tagPath, const char *tagName,
                          cfvariant *attrs, string *bodyContent, bool hasEndTag, bool isEndMode,
                          bool isModule, const char *templateNameHint)
{
    cfml::IncludeRuntime *rt = cfml::include_context();
    if (!rt || !rt->loader) {
        throw webstrada::exception("customtag", "Custom tag runtime is not available in this context.");
    }

    std::string pathStr;
    if (tagPathVar && tagPathVar->m_type != cfvariant::Null) {
        const string ps = variantToString(*tagPathVar);
        pathStr.assign(ps.constData(), ps.length());
    } else if (tagPath) {
        pathStr = tagPath;
    } else {
        throw webstrada::exception("customtag", "The tagPath argument is required.");
    }

    std::string resolved;
    if (!resolveCustomTagPath(pathStr, rt, resolved)) {
        throw webstrada::exception("customtag", ("Could not resolve the custom tag path " + pathStr + ".").c_str());
    }

    if (!customTagFileExists(resolved.c_str())) {
        // CF resolves imported-prefix tags at parse time ("Unknown tag:
        // prefix:tag."); here a missing template surfaces as a runtime error
        // with the same message shape. The taglib directory is stripped so the
        // reported name is the bare tag name.
        std::string display = pathStr;
        if (display.size() > 4 && display.compare(display.size() - 4, 4, ".cfm") == 0) {
            display = display.substr(0, display.size() - 4);
        }
        size_t slash = display.find_last_of('/');
        if (slash != std::string::npos) display = display.substr(slash + 1);
        throw webstrada::exception(("Unknown tag: " + display + ".").c_str());
    }
    include_template_fn target = rt->loader(resolved.c_str(), rt->loaderOpaque);
    if (!target) {
        throw webstrada::exception("customtag", ("Could not load custom tag template " + resolved + ".").c_str());
    }

    if (!isEndMode) {
        // For a cfmodule the public name (CF_<NAME>) is derived from the
        // resolved template's filename at runtime.
        std::string moduleHint;
        if (isModule) {
            moduleHint = resolved;
            size_t slash = moduleHint.find_last_of('/');
            if (slash != std::string::npos) moduleHint = moduleHint.substr(slash + 1);
            size_t dot = moduleHint.find_last_of('.');
            if (dot != std::string::npos) moduleHint = moduleHint.substr(0, dot);
            for (auto &c : moduleHint) c = toupper((unsigned char)c);
        }
        // Start mode
        cf_custom_tag_begin(tagName, attrs, hasEndTag, static_cast<cfvariant*>(variables),
                            isModule, moduleHint.empty() ? templateNameHint : moduleHint.c_str());
    } else {
        // End mode
        cf_custom_tag_end_mode(bodyContent);
    }

    std::string savedPath = rt->currentPath;
    rt->currentPath = resolved;
    rt->includeDepth++;

    CustomTagCallCtx &ctx = g_customTagStack.back();

    if (!isEndMode) {
        // Start template output goes straight to the caller's output buffer.
        try {
            target(out, cgi, server, cookie, application, session, url, form, &ctx.variables);
        } catch (const webstrada::exit_exception &ex) {
            // <cfexit> inside the start template: exittag/bare skips the body
            // and end tag; exittemplate only stops the start template (the
            // body still runs).
            if (ex.kind == 0) {
                ctx.skipBody = true;
            }
        } catch (...) {
            rt->currentPath = savedPath;
            rt->includeDepth--;
            throw;
        }
        rt->currentPath = savedPath;
        rt->includeDepth--;
        return;
    }

    // End mode. The end template's output is captured separately, and after it
    // runs the runtime emits the body (or the replacement generated content if
    // the end template assigned thisTag.generatedContent) followed by the end
    // template's own output — exactly CF's ModuleTag.doAfterBody.
    string *endOut = silent_buf_push();
    try {
        target(endOut, cgi, server, cookie, application, session, url, form, &ctx.variables);
    } catch (const webstrada::exit_exception &) {
        // <cfexit> in the end template stops it here (method="loop" is a
        // normal exit too; cf_exit_loop set loopRequested on the ctx before
        // throwing, which the caller checks).
    } catch (...) {
        silent_buf_pop();
        rt->currentPath = savedPath;
        rt->includeDepth--;
        throw;
    }

    if (ctx.contentChanged) {
        const string gen = variantToString(ctx.thisTag["GeneratedContent"]);
        out->append(gen.constData(), gen.length());
    } else if (bodyContent) {
        out->append(bodyContent->constData(), bodyContent->length());
    }
    out->append(endOut->constData(), endOut->length());

    silent_buf_pop();
    rt->currentPath = savedPath;
    rt->includeDepth--;
}

} // namespace cfml

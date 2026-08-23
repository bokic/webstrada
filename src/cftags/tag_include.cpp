/**
 * @file tag_include.cpp
 * @brief <cfinclude> runtime (cf_include + include path/loader helpers).
 */

#include "common.h"

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

// g_includeRuntime is defined in common.cpp.
bool includeResolvePath(const std::string &tpl, const cfml::IncludeRuntime *rt,
                        std::string &resolved)
{
    std::string normalizedTpl = tpl;
    for (auto &c : normalizedTpl) {
        if (c == '\\') c = '/';
    }
    // Check Application.cfc this.mappings first
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

bool includeFileExists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

cfml::include_template_fn includeLoadTemplate(const char *path, const cfml::IncludeRuntime *rt)
{
    if (!rt->loader || !includeFileExists(path)) return nullptr;
    return rt->loader(path, rt->loaderOpaque);
}

void includeStaticFile(string &out, const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw webstrada::exception("cfinclude", ("Could not read the included file " + path).c_str());
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string content = ss.str();
    out.append(content.c_str());
}

} // namespace

namespace cfml {

void cf_include(string *out, void *cgi, void *server, void *cookie, void *application,
                      void *session, void *url, void *form, void *variables,
                      const cfvariant *templatePath, const cfvariant *runonce)
{
    cfml::IncludeRuntime *rt = cfml::include_context();
    if (!rt || !rt->loader) {
        throw webstrada::exception("cfinclude", "cfinclude is not available in this context.");
    }
    if (!templatePath) {
        throw webstrada::exception("cfinclude", "The template attribute is required.");
    }

    string tplStr = const_cast<cfvariant*>(templatePath)->toString();
    std::string tpl = tplStr.constData() ? std::string(tplStr.constData(), tplStr.length()) : std::string();
    int runOnce = runonce ? cfml::cfvariant_is_truthy(runonce) : 0;
    std::string resolved;
    if (!includeResolvePath(tpl, rt, resolved)) {
        throw webstrada::exception("cfinclude", ("Could not resolve the template " + tpl + ".").c_str());
    }

    if (runOnce) {
        for (const auto &p : rt->runOnceIncluded) {
            if (p == resolved) {
                return;
            }
        }
    }

    std::string lower = resolved;
    for (auto &c : lower) c = (char)tolower((unsigned char)c);
    bool isCfml = lower.size() >= 4 &&
                  (lower.compare(lower.size() - 4, 4, ".cfm") == 0 ||
                   lower.compare(lower.size() - 5, 5, ".cfml") == 0);
    if (!isCfml) {
        rt->runOnceIncluded.push_back(resolved);
        includeStaticFile(*out, resolved);
        return;
    }

    include_template_fn target = includeLoadTemplate(resolved.c_str(), rt);
    if (!target) {
        throw webstrada::exception("MissingInclude",
                                  ("Could not find the included template " + tpl + ".").c_str(),
                                  "");
    }

    rt->runOnceIncluded.push_back(resolved);

    std::string savedPath = rt->currentPath;
    rt->currentPath = resolved;
    rt->includeDepth++;
    try {
        target(out, cgi, server, cookie, application, session, url, form, variables);
    } catch (const webstrada::exit_exception &) {
        // <cfexit> inside the included template exits only that page; the
        // caller continues (verified on CF: BEFORE<include>AFTER with a
        // <cfexit> in the include outputs BEFORE...AFTER).
    } catch (...) {
        rt->currentPath = savedPath;
        rt->includeDepth--;
        throw;
    }
    rt->currentPath = savedPath;
    rt->includeDepth--;
}

} // namespace cfml

/**
 * @file fn_trace.cpp
 * @brief CFML trace() built-in.
 *
 * CF's trace() maps to the <cftrace> tag as a function (TagsAsFunctions).
 * Attributes: var, text, type, category, inline, abort. The tag only executes
 * when debugging is enabled in the CF Administrator; with debugging disabled
 * (the default on the RDS verification host and in this engine, which has no
 * debug output section) it is a complete no-op — even abort="true" does not
 * fire. Verified against CF 2025: `trace()`, `trace(text="x", inline=true)`
 * and `trace(abort=true)` all render nothing / do not abort.
 *
 * Attribute names are validated (case-insensitive) like CF's TagsAsFunctions:
 * an unknown name throws "Attribute validation error for TRACE tag in
 * cfscript." / "It does not allow the attribute(s) X. The valid attribute(s)
 * are ABORT,CATEGORY,INLINE,TEXT,TYPE,VAR." (CF rejects the call at compile
 * time — uncatchable; this engine surfaces it at runtime, a documented
 * divergence). A positional argument list throws "Attribute validation error
 * for trace." (core_membermethods.cpp).
 *
 * The caller passes the named-argument marker struct (args[0] wrapping the
 * CFML_NAMED_ARGS_KEY struct), or nullptr for a bare `trace()` call.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace cfml {

using webstrada::cfvariant;
using webstrada::string;

static bool isTraceAttr(const std::string &name)
{
    static const char *valid[] = {"var", "text", "type", "category", "inline", "abort"};
    for (const char *v : valid) {
        if (name.size() == std::strlen(v)) {
            bool same = true;
            for (size_t i = 0; i < name.size(); i++) {
                if (tolower((unsigned char)name[i]) != v[i]) { same = false; break; }
            }
            if (same) return true;
        }
    }
    return false;
}

cfvariant *cf_trace(const cfvariant *namedArgs)
{
    // Unwrap the named-argument marker struct (CFML_NAMED_ARGS_KEY) that the
    // JIT passes for a named-argument call.
    const cfvariant *named = nullptr;
    if (namedArgs && namedArgs->m_type == cfvariant::Struct && namedArgs->m_struct) {
        auto it = namedArgs->m_struct->find(CFML_NAMED_ARGS_KEY);
        if (it != namedArgs->m_struct->end()) named = &it->second;
    }
    if (named && named->m_type == cfvariant::Struct && named->m_struct) {
        std::vector<std::string> unknown;
        for (const auto &kv : *named->m_struct) {
            if (!isTraceAttr(kv.first.constData())) unknown.push_back(kv.first.constData());
        }
        if (!unknown.empty()) {
            std::sort(unknown.begin(), unknown.end(),
                      [](const std::string &a, const std::string &b) {
                          std::string la, lb;
                          for (char c : a) la.push_back((char)toupper((unsigned char)c));
                          for (char c : b) lb.push_back((char)toupper((unsigned char)c));
                          return la < lb;
                      });
            std::string list;
            for (size_t i = 0; i < unknown.size(); i++) {
                if (i) list += ',';
                std::string up;
                for (char c : unknown[i]) up.push_back((char)toupper((unsigned char)c));
                list += up;
            }
            throw webstrada::exception("Template",
                webstrada::string("Attribute validation error for TRACE tag in cfscript."),
                webstrada::string(("It does not allow the attribute(s) " + list +
                                  ". The valid attribute(s) are ABORT,CATEGORY,INLINE,TEXT,TYPE,VAR.").c_str()));
        }
    }

    // With debugging disabled the <cftrace> tag executes nothing, so trace()
    // is a no-op returning an empty string value. (When debugging is enabled CF
    // records the trace and, for abort="true", aborts the request — that path
    // is not reproduced because this engine has no debug section.)
    auto *ret = new cfvariant("");
    return ret;
}

} // namespace cfml

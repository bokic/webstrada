/**
 * @file fn_structget.cpp
 * @brief CFML structget() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structget(const cfvariant *path, void *variables) {
    if (!path) throw webstrada::exception("StructGet requires a path argument");
    string p = const_cast<cfvariant*>(path)->toString();
    std::vector<string> parts = p.split('.');
    if (parts.empty() || parts[0].isEmpty()) {
        throw webstrada::exception("StructGet: Invalid path");
    }
    cfvariant *scope = static_cast<cfvariant*>(variables);
    // Scope-qualified paths (variables.a.b) drop the scope prefix.
    size_t start = 0;
    string first = parts[0];
    string firstUpper = first;
    firstUpper.toUpper();
    if (firstUpper.equals("VARIABLES") || firstUpper.equals("CGI") || firstUpper.equals("URL") ||
        firstUpper.equals("FORM") || firstUpper.equals("COOKIE") || firstUpper.equals("SERVER") ||
        firstUpper.equals("APPLICATION") || firstUpper.equals("SESSION")) {
        start = 1;
    }
    // Resolve (creating if needed) each segment in the variables scope; the
    // function returns the last struct (verified vs CF 2021).
    cfvariant *cur = scope;
    for (size_t i = start; i < parts.size(); i++) {
        if (cur->m_type != cfvariant::Struct) {
            throw webstrada::exception("StructGet: Path element is not a struct");
        }
        string key = parts[i];
        string keyUpper = key;
        keyUpper.toUpper();
        if (!cur->m_struct->contains(keyUpper)) {
            cur->structSet(keyUpper, cfvariant(cfvariant::Struct));
        }
        cur = &cur->m_struct->at(keyUpper);
    }
    auto *ret = new cfvariant(*cur);
    return ret;
}

} // namespace cfml

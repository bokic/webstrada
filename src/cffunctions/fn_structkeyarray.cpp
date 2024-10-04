/**
 * @file fn_structkeyarray.cpp
 * @brief CFML structkeyarray() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structkeyarray(const cfvariant *str) {
    if (!str) throw webstrada::exception("StructKeyArray: Missing argument");
    if (str->m_type != cfvariant::Struct && str->m_type != cfvariant::Component) {
        throw webstrada::exception("StructKeyArray: Argument must be a structure");
    }
    auto *ret = new cfvariant(cfvariant::Array);
    if (str->m_type == cfvariant::Component) {
        std::vector<webstrada::string> sortedKeys = getSortedComponentKeys(str, false, true, true);
        for (const auto &k : sortedKeys) ret->insert(cfvariant(k));
    } else if (str->m_isArguments) {
        for (const auto &k : argumentsVisibleKeys(str)) ret->insert(cfvariant(k));
    } else {
        for (auto &pair : *str->m_struct) {
            ret->insert(cfvariant(pair.first));
        }
    }
    return ret;
}

} // namespace cfml

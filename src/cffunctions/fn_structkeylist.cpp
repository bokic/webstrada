/**
 * @file fn_structkeylist.cpp
 * @brief CFML structkeylist() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structkeylist(const cfvariant *str, const cfvariant *delim) {
    if (!str) throw webstrada::exception("StructKeyList: Missing argument");
    if (str->m_type != cfvariant::Struct && str->m_type != cfvariant::Component) {
        throw webstrada::exception("StructKeyList: Argument must be a structure");
    }
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    string listStr;
    bool first = true;
    if (str->m_type == cfvariant::Component) {
        std::vector<webstrada::string> sortedKeys = getSortedComponentKeys(str, false, true, true);
        for (const auto &k : sortedKeys) {
            if (!first) listStr += d;
            listStr += k;
            first = false;
        }
    } else if (str->m_isArguments) {
        for (const auto &k : argumentsVisibleKeys(str)) {
            if (!first) listStr += d;
            listStr += k;
            first = false;
        }
    } else {
        for (auto &pair : *str->m_struct) {
            if (!first) listStr += d;
            listStr += pair.first;
            first = false;
        }
    }
    auto *ret = new cfvariant(listStr);
    return ret;
}

} // namespace cfml

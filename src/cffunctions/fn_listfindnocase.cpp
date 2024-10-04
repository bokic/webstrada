/**
 * @file fn_listfindnocase.cpp
 * @brief CFML listfindnocase() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listfindnocase(const cfvariant *list, const cfvariant *val, const cfvariant *delim) {
    if (!list || !val) throw webstrada::exception("ListFindNoCase: Missing argument(s)");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string target = const_cast<cfvariant*>(val)->toString();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    int foundIdx = 0;
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].compareCaseInsensitive(target) == 0) {
            foundIdx = static_cast<int>(i + 1);
            break;
        }
    }
    auto *ret = new cfvariant(foundIdx);
    return ret;
}

} // namespace cfml

/**
 * @file fn_listrest.cpp
 * @brief CFML listrest() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listrest(const cfvariant *list, const cfvariant *delim) {
    if (!list) throw webstrada::exception("ListRest: Missing argument");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    if (items.size() <= 1) {
        auto *ret = new cfvariant("");
        return ret;
    }
    items.erase(items.begin());
    auto *ret = new cfvariant(joinList(items, d));
    return ret;
}

} // namespace cfml

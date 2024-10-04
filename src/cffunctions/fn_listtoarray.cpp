/**
 * @file fn_listtoarray.cpp
 * @brief CFML listtoarray() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_listtoarray(const cfvariant *list, const cfvariant *delim) {
    if (!list) throw webstrada::exception("ListToArray: Missing argument");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    auto *ret = new cfvariant(cfvariant::Array);
    for (const auto &item : items) {
        ret->insert(cfvariant(item));
    }
    return ret;
}

} // namespace cfml

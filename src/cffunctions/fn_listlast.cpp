/**
 * @file fn_listlast.cpp
 * @brief CFML listlast() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listlast(const cfvariant *list, const cfvariant *delim) {
    if (!list) throw webstrada::exception("ListLast: Missing argument");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    auto *ret = new cfvariant(items.empty() ? "" : items.back().constData());
    return ret;
}

} // namespace cfml

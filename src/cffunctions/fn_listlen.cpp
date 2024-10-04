/**
 * @file fn_listlen.cpp
 * @brief CFML listlen() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listlen(const cfvariant *list, const cfvariant *delim) {
    if (!list) throw webstrada::exception("ListLen: Missing argument");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    auto *ret = new cfvariant(static_cast<int>(items.size()));
    return ret;
}

} // namespace cfml

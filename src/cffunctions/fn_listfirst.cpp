/**
 * @file fn_listfirst.cpp
 * @brief CFML listfirst() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listfirst(const cfvariant *list, const cfvariant *delim) {
    if (!list) throw webstrada::exception("ListFirst: Missing argument");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    auto *ret = new cfvariant(items.empty() ? "" : items.front().constData());
    return ret;
}

} // namespace cfml

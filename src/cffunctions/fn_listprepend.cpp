/**
 * @file fn_listprepend.cpp
 * @brief CFML listprepend() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listprepend(const cfvariant *list, const cfvariant *val, const cfvariant *delim) {
    if (!list || !val) throw webstrada::exception("ListPrepend: Missing argument(s)");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string valStr = const_cast<cfvariant*>(val)->toString().trimmed();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    if (!valStr.isEmpty()) {
        items.insert(items.begin(), valStr);
    }
    auto *ret = new cfvariant(joinList(items, d));
    return ret;
}

} // namespace cfml

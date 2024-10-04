/**
 * @file fn_listgetat.cpp
 * @brief CFML listgetat() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listgetat(const cfvariant *list, const cfvariant *idx, const cfvariant *delim) {
    if (!list || !idx) throw webstrada::exception("ListGetAt: Missing argument(s)");
    string listStr = const_cast<cfvariant*>(list)->toString();
    int index = getIntValue(*idx);
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    if (index < 1 || index > (int)items.size()) {
        throw webstrada::exception("ListGetAt: Index out of bounds");
    }
    auto *ret = new cfvariant(items[index - 1]);
    return ret;
}

} // namespace cfml

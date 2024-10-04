/**
 * @file fn_listdeleteat.cpp
 * @brief CFML listdeleteat() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listdeleteat(const cfvariant *list, const cfvariant *idx, const cfvariant *delim) {
    if (!list || !idx) throw webstrada::exception("ListDeleteAt: Missing argument(s)");
    string listStr = const_cast<cfvariant*>(list)->toString();
    int index = getIntValue(*idx);
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    if (index < 1 || index > (int)items.size()) {
        throw webstrada::exception("ListDeleteAt: Index out of bounds");
    }
    items.erase(items.begin() + (index - 1));
    auto *ret = new cfvariant(joinList(items, d));
    return ret;
}

} // namespace cfml

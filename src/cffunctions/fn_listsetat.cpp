/**
 * @file fn_listsetat.cpp
 * @brief CFML listsetat() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listsetat(const cfvariant *list, const cfvariant *idx, const cfvariant *val, const cfvariant *delim) {
    if (!list || !idx || !val) throw webstrada::exception("ListSetAt: Missing argument(s)");
    string listStr = const_cast<cfvariant*>(list)->toString();
    int index = getIntValue(*idx);
    string valStr = const_cast<cfvariant*>(val)->toString().trimmed();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    if (index < 1 || index > (int)items.size()) {
        throw webstrada::exception("ListSetAt: Index out of bounds");
    }
    items[index - 1] = valStr;
    auto *ret = new cfvariant(joinList(items, d));
    return ret;
}

} // namespace cfml

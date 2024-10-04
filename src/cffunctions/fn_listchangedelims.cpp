/**
 * @file fn_listchangedelims.cpp
 * @brief CFML listchangedelims() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listchangedelims(const cfvariant *list, const cfvariant *newDelim, const cfvariant *delim) {
    if (!list || !newDelim) throw webstrada::exception("ListChangeDelims: Missing argument(s)");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string nd = const_cast<cfvariant*>(newDelim)->toString();
    string d = ",";
    if (delim) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = parseList(listStr, d);
    auto *ret = new cfvariant(joinList(items, nd));
    return ret;
}

} // namespace cfml

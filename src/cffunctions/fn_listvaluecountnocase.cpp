/**
 * @file fn_listvaluecountnocase.cpp
 * @brief CFML listvaluecountnocase() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listvaluecountnocase(const cfvariant *list, const cfvariant *value, const cfvariant *delim) {
    if (!list || !value) throw webstrada::exception("ListValueCountNoCase requires a list and a value");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim && delim->m_type != cfvariant::Null) d = const_cast<cfvariant*>(delim)->toString();
    string target = const_cast<cfvariant*>(value)->toString();
    std::vector<string> items = splitList(listStr, d, false);
    int count = 0;
    for (const auto &item : items) if (item.compareCaseInsensitive(target) == 0) count++;
    auto *ret = new cfvariant(count);
    return ret;
}

} // namespace cfml

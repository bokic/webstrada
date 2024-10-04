/**
 * @file fn_listremoveduplicates.cpp
 * @brief CFML listremoveduplicates() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listremoveduplicates(const cfvariant *list, const cfvariant *delim, const cfvariant *ignoreCase) {
    if (!list) throw webstrada::exception("ListRemoveDuplicates requires a list");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim && delim->m_type != cfvariant::Null) d = const_cast<cfvariant*>(delim)->toString();
    bool nocase = ignoreCase ? isTruthy(*ignoreCase) : false;
    std::vector<string> items = splitList(listStr, d, false);
    std::vector<string> out;
    for (const auto &item : items) {
        bool dup = false;
        for (const auto &seen : out) {
            if (nocase ? seen.compareCaseInsensitive(item) == 0 : seen.equals(item)) { dup = true; break; }
        }
        if (!dup) out.push_back(item);
    }
    auto *ret = new cfvariant(joinListItems(out, d));
    return ret;
}

} // namespace cfml

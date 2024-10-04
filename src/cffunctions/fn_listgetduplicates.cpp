/**
 * @file fn_listgetduplicates.cpp
 * @brief CFML listgetduplicates() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listgetduplicates(const cfvariant *list, const cfvariant *delim) {
    if (!list) throw webstrada::exception("ListGetDuplicates requires a list");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim && delim->m_type != cfvariant::Null) d = const_cast<cfvariant*>(delim)->toString();
    std::vector<string> items = splitList(listStr, d, false);
    // Values that occur more than once, sorted alphabetically; CF joins them
    // with the input delimiter into a string (verified vs CF 2021).
    std::vector<string> dups;
    for (size_t i = 0; i < items.size(); i++) {
        bool already = false;
        for (const auto &x : dups) if (x.equals(items[i])) { already = true; break; }
        if (already) continue;
        size_t count = 0;
        for (const auto &x : items) if (x.equals(items[i])) count++;
        if (count > 1) dups.push_back(items[i]);
    }
    std::sort(dups.begin(), dups.end(), [](const string &a, const string &b) {
        return a.compareCaseInsensitive(b) < 0;
    });
    auto *ret = new cfvariant(joinListItems(dups, d));
    return ret;
}

} // namespace cfml

/**
 * @file fn_structtosorted.cpp
 * @brief CFML structtosorted() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structtosorted(const cfvariant *st, const cfvariant *sortType, const cfvariant *sortOrder,
                                   const cfvariant *localeSensitive,
                                   string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!st || st->m_type != cfvariant::Struct) throw webstrada::exception("StructToSorted: First argument must be a struct");
    std::vector<string> keys = structOrderedKeys(*st);
    if (sortType && sortType->m_type == cfvariant::Function) {
        // Callback form sorts by the struct's values (verified vs CF 2021).
        cfvariant cbVal = *sortType;
        std::stable_sort(keys.begin(), keys.end(), [&](const string &a, const string &b) {
            const cfvariant &va = st->m_struct->at(a);
            const cfvariant &vb = st->m_struct->at(b);
            std::vector<cfvariant> cbArgs = { va, vb };
            cfvariant res = callCallback(out, cbVal, cbArgs, cgi, server, cookie, application, session, url, form, variables);
            return getIntValue(res) < 0;
        });
    } else {
        // The non-callback form sorts the KEYS (verified vs CF 2021: "text").
        bool asc = true;
        if (sortOrder && sortOrder->m_type != cfvariant::Null) {
            string o = const_cast<cfvariant*>(sortOrder)->toString();
            o.toUpper();
            asc = !o.equals("DESC");
        }
        std::stable_sort(keys.begin(), keys.end(), [&](const string &a, const string &b) {
            return asc ? a.compareCaseInsensitive(b) < 0 : a.compareCaseInsensitive(b) > 0;
        });
    }
    auto *ret = new cfvariant(cfvariant::Struct);
    for (const auto &k : keys) {
        ret->structSet(k, st->m_struct->at(k));
    }
    return ret;
}

} // namespace cfml

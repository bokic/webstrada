/**
 * @file fn_structsort.cpp
 * @brief CFML structsort() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structsort(const cfvariant *st, const cfvariant *sortType, const cfvariant *sortOrder,
                               const cfvariant *path, const cfvariant *localeSensitive,
                               string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!st || st->m_type != cfvariant::Struct) throw webstrada::exception("StructSort: First argument must be a struct");
    std::vector<string> keys = structOrderedKeys(*st);
    if (sortType && sortType->m_type == cfvariant::Function) {
        // Callback form: CF passes the struct's values to the comparator
        // (verified vs CF 2021).
        cfvariant cbVal = *sortType;
        std::stable_sort(keys.begin(), keys.end(), [&](const string &a, const string &b) {
            const cfvariant &va = st->m_struct->at(a);
            const cfvariant &vb = st->m_struct->at(b);
            std::vector<cfvariant> cbArgs = { va, vb };
            cfvariant res = callCallback(out, cbVal, cbArgs, cgi, server, cookie, application, session, url, form, variables);
            return getIntValue(res) < 0;
        });
    } else {
        string typeStr = "text";
        if (sortType && sortType->m_type != cfvariant::Null) typeStr = const_cast<cfvariant*>(sortType)->toString();
        typeStr.toUpper();
        bool asc = true;
        if (sortOrder && sortOrder->m_type != cfvariant::Null) {
            string o = const_cast<cfvariant*>(sortOrder)->toString();
            o.toUpper();
            asc = !o.equals("DESC");
        }
        std::stable_sort(keys.begin(), keys.end(), [&](const string &a, const string &b) {
            const cfvariant &va = st->m_struct->at(a);
            const cfvariant &vb = st->m_struct->at(b);
            bool less = false;
            if (typeStr.equals("NUMERIC")) {
                less = getDoubleValue(va) < getDoubleValue(vb);
            } else if (typeStr.equals("TEXTNOCASE")) {
                less = variantToString(va).compareCaseInsensitive(variantToString(vb)) < 0;
            } else {
                less = variantToString(va) < variantToString(vb);
            }
            return asc ? less : !less;
        });
    }
    auto *ret = new cfvariant(cfvariant::Array);
    for (const auto &k : keys) ret->insert(cfvariant(k));
    return ret;
}

} // namespace cfml

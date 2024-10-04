/**
 * @file fn_listsort.cpp
 * @brief CFML listsort() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listsort(const cfvariant *list, const cfvariant *sortType, const cfvariant *sortOrder,
                       const cfvariant *delimiters, const cfvariant *includeEmptyFields, const cfvariant *localeSensitive,
                       string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!list) throw webstrada::exception("ListSort requires a list");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delimiters && delimiters->m_type != cfvariant::Null) d = const_cast<cfvariant*>(delimiters)->toString();
    bool keepEmpty = includeEmptyFields ? isTruthy(*includeEmptyFields) : false;
    std::vector<string> items = splitList(listStr, d, keepEmpty);

    if (sortType && sortType->m_type == cfvariant::Function) {
        // Callback form: sortType holds the comparator closure.
        cfvariant cbVal = *sortType;
        std::stable_sort(items.begin(), items.end(), [&](const string &a, const string &b) {
            std::vector<cfvariant> cbArgs = { cfvariant(a), cfvariant(b) };
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
        std::stable_sort(items.begin(), items.end(), [&](const string &a, const string &b) {
            bool less = false;
            if (typeStr.equals("NUMERIC")) {
                less = getDoubleValue(cfvariant(a)) < getDoubleValue(cfvariant(b));
            } else if (typeStr.equals("TEXTNOCASE")) {
                less = a.compareCaseInsensitive(b) < 0;
            } else {
                // CF's "text" sort is case-SENSITIVE (verified on CF 2025:
                // ListSort("NAME,add","text") -> "NAME,add" because uppercase
                // letters sort before lowercase in ASCII).
                const char *ca = a.constData();
                const char *cb = b.constData();
                less = strcmp(ca ? ca : "", cb ? cb : "") < 0;
            }
            return asc ? less : !less;
        });
    }
    auto *ret = new cfvariant(joinListItems(items, d));
    return ret;
}

} // namespace cfml

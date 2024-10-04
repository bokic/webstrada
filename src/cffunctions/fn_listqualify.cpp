/**
 * @file fn_listqualify.cpp
 * @brief CFML listqualify() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listqualify(const cfvariant *list, const cfvariant *qualifier, const cfvariant *delimiters,
                          const cfvariant *elements, const cfvariant *includeEmptyValues) {
    if (!list || !qualifier) throw webstrada::exception("ListQualify requires a list and a qualifier");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delimiters && delimiters->m_type != cfvariant::Null) d = const_cast<cfvariant*>(delimiters)->toString();
    bool keepEmpty = false;
    if (includeEmptyValues) keepEmpty = isTruthy(*includeEmptyValues);
    std::vector<string> items = splitList(listStr, d, keepEmpty);
    string q = const_cast<cfvariant*>(qualifier)->toString();
    // CF's elements argument accepts "all" or "char" (both qualify everything);
    // the single-quote qualifier is the common case. A two-char qualifier is a
    // (prefix, suffix) pair; a one-char qualifier wraps both sides.
    string qpre = q, qsuf = q;
    if (q.length() >= 2) {
        qpre = q.left(1);
        qsuf = q.mid(1, q.length() - 1);
    }
    std::vector<string> qualified;
    for (const auto &item : items) {
        qualified.push_back(qpre + item + qsuf);
    }
    auto *ret = new cfvariant(joinListItems(qualified, d));
    return ret;
}

} // namespace cfml

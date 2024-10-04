/**
 * @file fn_listeach.cpp
 * @brief CFML listeach() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listeach(const cfvariant *list, const cfvariant *callback, const cfvariant *delim, const cfvariant *includeEmptyFields,
                       string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!list || !callback) throw webstrada::exception("ListEach requires a list and a callback");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim && delim->m_type != cfvariant::Null) d = const_cast<cfvariant*>(delim)->toString();
    bool keepEmpty = includeEmptyFields ? isTruthy(*includeEmptyFields) : false;
    std::vector<string> items = splitList(listStr, d, keepEmpty);
    cfvariant listVal(listStr);
    for (size_t i = 0; i < items.size(); i++) {
        std::vector<cfvariant> cbArgs = { cfvariant(items[i]), cfvariant(static_cast<int>(i + 1)), listVal };
        callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
    }
    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

} // namespace cfml

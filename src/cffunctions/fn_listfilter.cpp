/**
 * @file fn_listfilter.cpp
 * @brief CFML listfilter() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <string>
#include <vector>

using webstrada::string;

namespace cfml {

cfvariant *cf_listfilter(const cfvariant *list, const cfvariant *callback, const cfvariant *delim, const cfvariant *includeEmptyFields,
                         string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!list || !callback) throw webstrada::exception("ListFilter requires a list and a callback");
    string listStr = const_cast<cfvariant*>(list)->toString();
    string d = ",";
    if (delim && delim->m_type != cfvariant::Null) d = const_cast<cfvariant*>(delim)->toString();
    bool keepEmpty = includeEmptyFields ? isTruthy(*includeEmptyFields) : false;
    std::vector<string> items = splitList(listStr, d, keepEmpty);
    cfvariant listVal(listStr);
    std::vector<string> kept;
    for (size_t i = 0; i < items.size(); i++) {
        std::vector<cfvariant> cbArgs = { cfvariant(items[i]), cfvariant(static_cast<int>(i + 1)), listVal };
        cfvariant res = callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
        if (isTruthy(res)) kept.push_back(items[i]);
    }
    auto *ret = new cfvariant(joinListItems(kept, d));
    return ret;
}

} // namespace cfml

/**
 * @file fn_arrayfilter.cpp
 * @brief CFML arrayfilter() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayfilter(const cfvariant *arr, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                                string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!arr || arr->m_type != cfvariant::Array) throw webstrada::exception("ArrayFilter: First argument must be an array");
    if (!callback) throw webstrada::exception("ArrayFilter requires a callback");
    cfvariant arrVal = *arr;
    auto *ret = new cfvariant(cfvariant::Array);
    for (size_t i = 0; i < arr->m_array->size(); i++) {
        std::vector<cfvariant> cbArgs = { arr->m_array->at(i), cfvariant(static_cast<int>(i + 1)), arrVal };
        cfvariant keep = callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
        if (isTruthy(keep)) ret->insert(arr->m_array->at(i));
    }
    return ret;
}

} // namespace cfml

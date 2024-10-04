/**
 * @file fn_arrayreduce.cpp
 * @brief CFML arrayreduce() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arrayreduce(const cfvariant *arr, const cfvariant *callback, const cfvariant *initialValue,
                                string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!arr || arr->m_type != cfvariant::Array) throw webstrada::exception("ArrayReduce: First argument must be an array");
    if (!callback) throw webstrada::exception("ArrayReduce requires a callback");
    cfvariant arrVal = *arr;
    cfvariant acc = initialValue ? *initialValue : cfvariant(cfvariant::Null);
    for (size_t i = 0; i < arr->m_array->size(); i++) {
        std::vector<cfvariant> cbArgs = { acc, arr->m_array->at(i), cfvariant(static_cast<int>(i + 1)), arrVal };
        acc = callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
    }
    auto *ret = new cfvariant(acc);
    return ret;
}

} // namespace cfml

/**
 * @file fn_structfilter.cpp
 * @brief CFML structfilter() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structfilter(const cfvariant *st, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                                 string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!st || st->m_type != cfvariant::Struct) throw webstrada::exception("StructFilter: First argument must be a struct");
    if (!callback) throw webstrada::exception("StructFilter requires a callback");
    cfvariant structVal = *st;
    auto *ret = new cfvariant(cfvariant::Struct);
    for (const auto &key : structOrderedKeys(*st)) {
        const cfvariant &val = st->m_struct->at(key);
        std::vector<cfvariant> cbArgs = { cfvariant(key), val, structVal };
        cfvariant keep = callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
        if (isTruthy(keep)) ret->structSet(key, val);
    }
    return ret;
}

} // namespace cfml

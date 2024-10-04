/**
 * @file fn_structeach.cpp
 * @brief CFML structeach() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structeach(const cfvariant *st, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                               string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!st || st->m_type != cfvariant::Struct) throw webstrada::exception("StructEach: First argument must be a struct");
    if (!callback) throw webstrada::exception("StructEach requires a callback");
    cfvariant structVal = *st;
    for (const auto &key : structOrderedKeys(*st)) {
        const cfvariant &val = st->m_struct->at(key);
        std::vector<cfvariant> cbArgs = { cfvariant(key), val, structVal };
        callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
    }
    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

} // namespace cfml

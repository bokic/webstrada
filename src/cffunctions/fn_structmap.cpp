/**
 * @file fn_structmap.cpp
 * @brief CFML structmap() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structmap(const cfvariant *st, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                              string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!st || st->m_type != cfvariant::Struct) throw webstrada::exception("StructMap: First argument must be a struct");
    if (!callback) throw webstrada::exception("StructMap requires a callback");
    cfvariant structVal = *st;
    auto *ret = new cfvariant(cfvariant::Struct);
    for (const auto &key : structOrderedKeys(*st)) {
        const cfvariant &val = st->m_struct->at(key);
        std::vector<cfvariant> cbArgs = { cfvariant(key), val, structVal };
        cfvariant mapped = callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
        ret->structSet(key, mapped);
    }
    return ret;
}

} // namespace cfml

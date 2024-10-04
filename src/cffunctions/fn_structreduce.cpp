/**
 * @file fn_structreduce.cpp
 * @brief CFML structreduce() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structreduce(const cfvariant *st, const cfvariant *callback, const cfvariant *initialValue,
                                 string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!st || st->m_type != cfvariant::Struct) throw webstrada::exception("StructReduce: First argument must be a struct");
    if (!callback) throw webstrada::exception("StructReduce requires a callback");
    cfvariant structVal = *st;
    cfvariant acc = initialValue ? *initialValue : cfvariant(cfvariant::Null);
    for (const auto &key : structOrderedKeys(*st)) {
        const cfvariant &val = st->m_struct->at(key);
        std::vector<cfvariant> cbArgs = { acc, cfvariant(key), val, structVal };
        acc = callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
    }
    auto *ret = new cfvariant(acc);
    return ret;
}

} // namespace cfml

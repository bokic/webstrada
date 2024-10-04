/**
 * @file fn_queryreduce.cpp
 * @brief CFML queryreduce() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_queryreduce(const cfvariant *query, const cfvariant *callback, const cfvariant *initialValue,
                                string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryReduce: First argument must be a query");
    }
    if (!callback) throw webstrada::exception("QueryReduce requires a callback");
    QueryData *qd = query->m_query;
    int rc = qd->rowCount();
    cfvariant qval = *query;
    // Without an explicit initial value CF starts the accumulator as an
    // undefined (null) variable on the first call (verified vs CF 2021:
    // `acc + row.id` then throws "Variable ACC is undefined.").
    cfvariant acc = initialValue ? *initialValue : cfvariant(cfvariant::Null);
    for (int r = 0; r < rc; r++) {
        cfvariant row = queryBuildRowStruct(qd, r);
        std::vector<cfvariant> cbArgs = { acc, row, cfvariant(r + 1), qval };
        acc = callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
    }
    auto *ret = new cfvariant(acc);
    return ret;
}

} // namespace cfml

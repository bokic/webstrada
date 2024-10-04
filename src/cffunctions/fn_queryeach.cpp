/**
 * @file fn_queryeach.cpp
 * @brief CFML queryeach() built-in.
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

cfvariant *cf_queryeach(const cfvariant *query, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                              string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryEach: First argument must be a query");
    }
    if (!callback) throw webstrada::exception("QueryEach requires a callback");
    QueryData *qd = query->m_query;
    int rc = qd->rowCount();
    cfvariant qval = *query;
    for (int r = 0; r < rc; r++) {
        cfvariant row = queryBuildRowStruct(qd, r);
        std::vector<cfvariant> cbArgs = { row, cfvariant(r + 1), qval };
        callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
    }
    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

} // namespace cfml

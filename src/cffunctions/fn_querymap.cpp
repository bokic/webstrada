/**
 * @file fn_querymap.cpp
 * @brief CFML querymap() built-in.
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

cfvariant *cf_querymap(const cfvariant *query, const cfvariant *callback, const cfvariant *parallel, const cfvariant *maxThreads,
                             string &out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryMap: First argument must be a query");
    }
    if (!callback) throw webstrada::exception("QueryMap requires a callback");
    QueryData *qd = query->m_query;
    int rc = qd->rowCount();
    cfvariant qval = *query;

    auto *ret = new cfvariant(cfvariant::Query);
    ret->m_query->columns.clear();
    for (const auto &col : qd->columns) {
        QueryColumn nc;
        nc.name = col.name;
        nc.type = col.type;
        ret->m_query->columns.push_back(nc);
    }
    for (int r = 0; r < rc; r++) {
        cfvariant row = queryBuildRowStruct(qd, r);
        std::vector<cfvariant> cbArgs = { row, cfvariant(r + 1), qval };
        cfvariant mapped = callCallback(out, *callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
        if (mapped.m_type != cfvariant::Struct || !mapped.m_struct) {
            throw webstrada::exception("QueryMap: Callback must return a struct");
        }
        for (size_t c = 0; c < qd->columns.size(); c++) {
            string colName = qd->columns[c].name;
            auto it = mapped.m_struct->find(colName);
            if (it == mapped.m_struct->end()) {
                throw webstrada::exception("QueryMap: Callback result is missing the column '" + variantToString(colName) + "'");
            }
            ret->m_query->columns[c].values.push_back(it->second);
        }
        ret->m_query->m_rowCount++;
    }
    return ret;
}

} // namespace cfml

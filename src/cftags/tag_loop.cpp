/**
 * @file tag_loop.cpp
 * @brief <cfloop query> runtime (query-scope push/pop, row navigation, group).
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <string>
#include <vector>

namespace cfml {

long long cf_query_rowcount(const cfvariant *query)
{
    if (!query || query->m_type != cfvariant::Query || !query->m_query) return 0;
    return query->m_query->rowCount();
}

cfvariant *cf_query_resolve(const cfvariant *value,
                            void *cgi, void *server, void *cookie, void *application,
                            void *session, void *url, void *form, void *variables)
{
    const cfvariant *q = value;
    if (value && (value->m_type == cfvariant::String || value->m_type == cfvariant::Number)) {
        std::string name = safe_to_std_string(*value);
        q = lookupVarWritable(name.c_str(),
            static_cast<cfvariant*>(cgi), static_cast<cfvariant*>(server),
            static_cast<cfvariant*>(cookie), static_cast<cfvariant*>(application),
            static_cast<cfvariant*>(session), static_cast<cfvariant*>(url),
            static_cast<cfvariant*>(form), static_cast<cfvariant*>(variables));
    }
    if (!q || q->m_type != cfvariant::Query || !q->m_query) {
        throw webstrada::exception("cfquery loop", "The query attribute must be a query object.");
    }
    return const_cast<cfvariant*>(q);
}

void cf_query_set_row(const cfvariant *query, long long row)
{
    if (query && query->m_type == cfvariant::Query && query->m_query) {
        query->m_query->currentRow = static_cast<int>(row);
    }
}

long long cf_query_scope_push(const cfvariant *query)
{
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("cfquery loop", "The query attribute must be a query object.");
    }
    // The scope owns its own copy so a loop that unwinds without popping (an
    // exception in the body) cannot leave a dangling pointer to a scope slot
    // that is freed when the request/template ends (was a use-after-free once
    // the engine's temp-variant leaks were fixed). The copy shares the QueryData
    // payload (retained), so cursor/row mutations stay visible to the original.
    g_queryScopes.push_back(new cfvariant(*query));
    return query->m_query->currentRow;
}

void cf_query_scope_pop()
{
    if (!g_queryScopes.empty()) {
        delete g_queryScopes.back();
        g_queryScopes.pop_back();
    }
}

long long cf_query_group_next(const cfvariant *query, const cfvariant *groupCol,
                              long long currentRow, long long endRow,
                              const cfvariant *caseSensitive)
{
    if (!query || query->m_type != cfvariant::Query || !query->m_query) return endRow + 1;
    QueryData *qd = query->m_query;
    string colName = groupCol ? variantToString(*groupCol) : string("");
    int colIdx = qd->findColumn(colName);
    if (colIdx < 0) {
        throw webstrada::exception("cfloop", ("The query does not contain a column named '" +
            safe_to_std_string(colName) + "' to group on.").c_str());
    }

    auto getCell = [&](long long r) -> cfvariant {
        if (r < 1 || r > (long long)qd->columns[colIdx].values.size()) return cfvariant(cfvariant::Null);
        return qd->columns[colIdx].values[r - 1];
    };

    bool caseSens = caseSensitive ? cfvariant_is_truthy(caseSensitive) : false;
    cfvariant curVal = getCell(currentRow);

    auto matches = [&](const cfvariant &a, const cfvariant &b) -> bool {
        return caseSens ? cfvariantsEqual(a, b) : cfvariantsEqualNoCase(a, b);
    };

    for (long long r = currentRow + 1; r <= endRow; r++) {
        if (!matches(curVal, getCell(r))) {
            return r;
        }
    }
    return endRow + 1;
}

} // namespace cfml

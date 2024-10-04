/**
 * @file fn_queryconvertforgrid.cpp
 * @brief CFML queryconvertforgrid() built-in.
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

cfvariant *cf_queryconvertforgrid(const cfvariant *query, const cfvariant *page, const cfvariant *pageSize) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryConvertForGrid: First argument must be a query");
    }
    if (!page || !pageSize) {
        throw webstrada::exception("QueryConvertForGrid requires page and pageSize arguments");
    }
    QueryData *qd = query->m_query;
    int p = getIntValue(*page);
    int ps = getIntValue(*pageSize);
    int rc = qd->rowCount();
    int start = (p - 1) * ps;
    if (start < 0) {
        std::string msg = "Index " + std::to_string(start) + " out of bounds for length " + std::to_string(rc);
        throw webstrada::exception(msg.c_str());
    }

    // Paged subset: rows [start, start + ps), clamped to the record count; a
    // start at/after the last row yields an empty query (verified vs CF 2021).
    auto *subset = new cfvariant(cfvariant::Query);
    subset->m_query->columns.clear();
    for (const auto &col : qd->columns) {
        QueryColumn nc;
        nc.name = col.name;
        nc.type = col.type;
        subset->m_query->columns.push_back(nc);
    }
    int count = ps;
    if (count > rc - start) count = rc - start;
    if (count < 0) count = 0;
    for (int r = 0; r < count; r++) {
        for (size_t c = 0; c < qd->columns.size(); c++) {
            subset->m_query->columns[c].values.push_back(qd->columns[c].values[start + r]);
        }
        subset->m_query->m_rowCount++;
    }

    auto *ret = new cfvariant(cfvariant::Struct);
    ret->structSet("QUERY", *subset);
    cf_register_temp(subset);
    ret->structSet("TOTALROWCOUNT", cfvariant(rc));
    return ret;
}

} // namespace cfml

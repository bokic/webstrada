/**
 * @file fn_querysetcell.cpp
 * @brief CFML querysetcell() built-in.
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

cfvariant *cf_querysetcell(cfvariant *query, const cfvariant *column, const cfvariant *value, const cfvariant *row) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QuerySetCell: First argument must be a query");
    }
    if (!column) throw webstrada::exception("QuerySetCell requires a column name");
    if (!value) throw webstrada::exception("QuerySetCell requires a value");

    QueryData *qd = query->m_query;
    string colName = variantToString(*column);
    int colIdx = qd->findColumn(colName);
    if (colIdx < 0) {
        throw webstrada::exception("QuerySetCell: Column '" + colName + "' does not exist in query");
    }
    int rc = qd->rowCount();
    if (rc == 0) {
        throw webstrada::exception("QuerySetCell: Cannot set a cell in a query with no rows");
    }

    int targetRow;
    if (!row || row->m_type == cfvariant::Null) {
        // No row: use the last row (verified against CF 2021).
        targetRow = rc;
    } else {
        targetRow = getIntValue(*row);
        // Negative row numbers behave like "no row" (last row); 0 and
        // out-of-bounds positive rows throw (verified against CF 2021).
        if (targetRow < 0) targetRow = rc;
        else if (targetRow < 1 || targetRow > rc) {
            throw webstrada::exception("QuerySetCell: Row number " + variantToString(*row) + " is out of bounds");
        }
    }

    QueryColumn &col = qd->columns[colIdx];
    col.values[targetRow - 1] = coerceQueryCell(col.type, *value);

    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml

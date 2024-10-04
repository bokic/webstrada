/**
 * @file fn_queryaddcolumn.cpp
 * @brief CFML queryaddcolumn() built-in.
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

cfvariant *cf_queryaddcolumn(cfvariant *query, const cfvariant *column_name, const cfvariant *datatype_or_array, const cfvariant *array_name) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryAddColumn: First argument must be a query");
    }
    if (!column_name) throw webstrada::exception("QueryAddColumn requires a column name");

    const cfvariant *arr = array_name ? array_name : datatype_or_array;
    string colType = "VARCHAR";
    if (array_name && datatype_or_array) {
        colType = variantToString(*datatype_or_array);
    }
    if (!arr || arr->m_type != cfvariant::Array) {
        throw webstrada::exception("QueryAddColumn: Last argument must be an array");
    }

    QueryData *qd = query->m_query;
    string name = variantToString(*column_name);
    string nameUpper = name;
    nameUpper.toUpper();

    if (qd->findColumn(nameUpper) >= 0) {
        throw webstrada::exception("QueryAddColumn: Column '" + name + "' already exists in query");
    }

    // Add the column; its cell count may differ from the existing columns.
    QueryColumn col;
    col.name = name;
    colType.toUpper();
    colType = colType.trimmed();
    if (!colType.isEmpty()) col.type = colType;
    else col.type = "VARCHAR";
    for (auto &v : *arr->m_array) {
        col.values.push_back(coerceQueryCell(col.type, v));
    }
    qd->columns.push_back(col);

    // Pad all columns (including the new one) to the same row count.
    int maxRows = 0;
    for (auto &c : qd->columns) {
        if ((int)c.values.size() > maxRows) maxRows = (int)c.values.size();
    }
    for (auto &c : qd->columns) {
        if ((int)c.values.size() < maxRows) {
            c.values.resize(maxRows, cfvariant(cfvariant::Null));
        }
    }
    if (maxRows > qd->m_rowCount) qd->m_rowCount = maxRows;

    auto *ret = new cfvariant((int)qd->columns.size());
    return ret;
}

} // namespace cfml

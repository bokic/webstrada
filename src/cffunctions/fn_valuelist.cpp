/**
 * @file fn_valuelist.cpp
 * @brief CFML valuelist() built-in.
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

cfvariant *cf_valuelist(const cfvariant *column, const cfvariant *delimiter) {
    if (!column) throw webstrada::exception("ValueList requires a column argument");
    string delim = ",";
    if (delimiter) {
        if (delimiter->m_type != cfvariant::Null) delim = variantToString(*delimiter);
    }
    string result;
    if (column->m_type == cfvariant::Array && column->m_array) {
        for (size_t i = 0; i < column->m_array->size(); i++) {
            const cfvariant &cell = (*column->m_array)[i];
            if (i > 0) result += delim;
            if (cell.m_type != cfvariant::Null) {
                result += variantToString(cell);
            }
        }
    } else if (column->m_type == cfvariant::Query && column->m_query && !column->m_query->columns.empty()) {
        // ValueList(q) without a column name: CF resolves the "current" column
        // this way only in query contexts; accept the first column as a fallback.
        const QueryColumn &col = column->m_query->columns[0];
        for (size_t i = 0; i < col.values.size(); i++) {
            if (i > 0) result += delim;
            if (col.values[i].m_type != cfvariant::Null) result += variantToString(col.values[i]);
        }
    } else {
        throw webstrada::exception("ValueList: Argument must be a query column");
    }
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml

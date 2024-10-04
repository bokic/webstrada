/**
 * @file fn_quotedvaluelist.cpp
 * @brief CFML quotedvaluelist() built-in.
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

cfvariant *cf_quotedvaluelist(const cfvariant *column, const cfvariant *delimiter) {
    if (!column) throw webstrada::exception("QuotedValueList requires a column argument");
    string delim = ",";
    if (delimiter && delimiter->m_type != cfvariant::Null) delim = variantToString(*delimiter);

    string result;
    auto quoteCell = [&result](const cfvariant &cell) {
        result += "'" + variantToString(cell) + "'";
    };

    if (column->m_type == cfvariant::Array && column->m_array) {
        for (size_t i = 0; i < column->m_array->size(); i++) {
            if (i > 0) result += delim;
            quoteCell((*column->m_array)[i]);
        }
    } else if (column->m_type == cfvariant::Query && column->m_query && !column->m_query->columns.empty()) {
        // A whole query without a column name is rejected by CF (verified vs
        // CF 2021); only an explicit column reference (q.col, materialized as
        // an array by this engine) is supported.
        throw webstrada::exception("QuotedValueList: Argument must be a query column");
    } else {
        throw webstrada::exception("QuotedValueList: Argument must be a query column");
    }
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml

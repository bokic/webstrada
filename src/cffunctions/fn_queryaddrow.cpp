/**
 * @file fn_queryaddrow.cpp
 * @brief CFML queryaddrow() built-in.
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

static void validateQueryRowKeys(const QueryData *qd, const cfvariant &row)
{
    if (!qd || row.m_type != cfvariant::Struct || !row.m_struct) return;
    for (auto &kv : *row.m_struct) {
        string k = kv.first;
        k.toUpper();
        if (qd->findColumn(k) < 0) {
            throw webstrada::exception("QueryAddRow: Column '" + kv.first + "' does not exist in query");
        }
    }
}

cfvariant *cf_queryaddrow(cfvariant *query, const cfvariant *rows) {
    if (!query || query->m_type != cfvariant::Query || !query->m_query) {
        throw webstrada::exception("QueryAddRow: First argument must be a query");
    }
    QueryData *qd = query->m_query;

    if (!rows || rows->m_type == cfvariant::Null) {
        qd->addEmptyRow();
    } else if (rows->m_type == cfvariant::Number || rows->m_type == cfvariant::Long ||
               rows->m_type == cfvariant::Float || rows->m_type == cfvariant::Boolean) {
        int n = getIntValue(*rows);
        if (n < 0) throw webstrada::exception("QueryAddRow: Row count cannot be negative");
        for (int i = 0; i < n; i++) qd->addEmptyRow();
    } else if (rows->m_type == cfvariant::Struct) {
        // A struct inserts a single row keyed by column name; a key that is
        // not a declared column throws (CF Expression error, verified).
        validateQueryRowKeys(qd, *rows);
        std::vector<cfvariant> values(qd->columns.size(), cfvariant(cfvariant::Null));
        for (size_t c = 0; c < qd->columns.size(); c++) {
            string up = qd->columns[c].name;
            up.toUpper();
            auto it = rows->m_struct->find(up);
            if (it != rows->m_struct->end()) values[c] = it->second;
        }
        qd->addRow(values);
    } else if (rows->m_type == cfvariant::Array) {
        bool rowsAreStructs = false;
        if (!rows->m_array->empty() && (*rows->m_array)[0].m_type == cfvariant::Struct) {
            rowsAreStructs = true;
        }
        for (size_t r = 0; r < rows->m_array->size(); r++) {
            const cfvariant &row = (*rows->m_array)[r];
            if (rowsAreStructs && row.m_type == cfvariant::Struct) {
                validateQueryRowKeys(qd, row);
                std::vector<cfvariant> values(qd->columns.size(), cfvariant(cfvariant::Null));
                for (size_t c = 0; c < qd->columns.size(); c++) {
                    string up = qd->columns[c].name;
                    up.toUpper();
                    auto it = row.m_struct->find(up);
                    if (it != row.m_struct->end()) values[c] = it->second;
                }
                qd->addRow(values);
            } else {
                // A scalar/positional element in the row array is not valid
                // row data; CF throws (verified against CF 2021).
                throw webstrada::exception("QueryAddRow: Invalid row data");
            }
        }
    } else {
        // A non-numeric string is not a valid row count (CF: NumberFormatException).
        throw webstrada::exception("QueryAddRow: Invalid row count");
    }

    auto *ret = new cfvariant(qd->rowCount());
    return ret;
}

} // namespace cfml

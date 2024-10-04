/**
 * @file fn_querynew.cpp
 * @brief CFML querynew() built-in.
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

static std::vector<string> splitCsv(const string &s, char delim)
{
    std::vector<string> out;
    const char *p = s.constData();
    if (!p) return out;
    std::string cur;
    for (const char *c = p; *c; c++) {
        if (*c == delim) {
            out.push_back(string(cur.c_str()));
            cur.clear();
        } else {
            cur += *c;
        }
    }
    out.push_back(string(cur.c_str()));
    return out;
}

cfvariant *cf_querynew(const cfvariant *columnList, const cfvariant *columnTypeList, const cfvariant *rowData) {
    if (!columnList) {
        throw webstrada::exception("QueryNew requires at least 1 argument");
    }

    auto *ret = new cfvariant(cfvariant::Query);

    std::vector<string> colNames;         // uppercased, for case-insensitive lookup
    std::vector<string> colOrigNames;     // original case, for cfdump/serializeJSON
    std::vector<string> colTypes;
    std::vector<cfvariant> initialRow;  // populated when columnList is a struct

    // columnList is a comma-delimited string. An array or other complex first
    // argument throws in CF (QueryNew(["aa","bb"]) -> ClassCastException), so
    // it is rejected here rather than stringified.
    if (columnList->m_type != cfvariant::Struct) {
        if (columnList->m_type != cfvariant::String && columnList->m_type != cfvariant::Number &&
            columnList->m_type != cfvariant::Float && columnList->m_type != cfvariant::Long &&
            columnList->m_type != cfvariant::Boolean && columnList->m_type != cfvariant::DateTime &&
            columnList->m_type != cfvariant::Null) {
            throw webstrada::exception("QueryNew: Invalid columnList argument");
        }
        string names = variantToString(*columnList);
        std::vector<string> pieces = splitCsv(names, ',');
        for (auto &p : pieces) {
            string n = p.trimmed();
            if (!n.isEmpty()) {
                colOrigNames.push_back(n);
                n.toUpper();
                colNames.push_back(n);
            }
        }
    }

    if (columnList->m_type == cfvariant::Struct && columnList->m_struct) {
        // CF treats a struct first argument as row data, not as a name->type
        // map (verified against CF 2021: QueryNew({a:"integer", b:"varchar"})
        // yields columns A,B with a single row whose cells are the struct's
        // values, all varchar). The struct's insertion order is preserved and
        // the column list is still reported sorted.
        for (auto &kv : *columnList->m_struct) {
            string name = kv.first;
            colOrigNames.push_back(name);
            name.toUpper();
            colNames.push_back(name);
        }
        for (auto &kv : *columnList->m_struct) {
            initialRow.push_back(kv.second);
        }
    }

    if (columnTypeList) {
        string types = variantToString(*columnTypeList);
        types.toUpper();
        std::vector<string> tpieces = splitCsv(types, ',');
        for (auto &p : tpieces) {
            string n = p.trimmed();
            if (!n.isEmpty()) colTypes.push_back(n);
        }
    }
    // Default column type is varchar; pad a short type list (CF ignores extra
    // types beyond the column count).
    while (colTypes.size() < colNames.size()) colTypes.push_back("VARCHAR");

    // Determine how many rows the query starts with and the per-cell data.
    std::vector<std::vector<cfvariant>> cells;  // [colIndex][rowIndex]
    cells.resize(colNames.size());

    int rowCount = 0;
    auto addRow = [&](const std::vector<cfvariant> &values) {
        for (size_t c = 0; c < colNames.size(); c++) {
            if (c < values.size()) {
                cells[c].push_back(coerceQueryCell(colTypes[c], values[c]));
            } else {
                cfvariant n(cfvariant::Null);
                cells[c].push_back(n);
            }
        }
        rowCount++;
    };

    if (columnList->m_type == cfvariant::Struct && columnList->m_struct) {
        // The struct first argument itself provides the single initial row.
        addRow(initialRow);
    } else if (rowData) {
        if (rowData->m_type == cfvariant::Struct) {
            // single row keyed by column name (case-insensitive). A struct key
            // that is not a declared column throws in CF (Expression error).
            for (auto &kv : *rowData->m_struct) {
                string k = kv.first;
                k.toUpper();
                bool found = false;
                for (auto &cn : colNames) {
                    if (cn.equals(k)) { found = true; break; }
                }
                if (!found) {
                    throw webstrada::exception("QueryNew: Column '" + kv.first + "' is not in the query's column list");
                }
            }
            std::vector<cfvariant> values(colNames.size(), cfvariant(cfvariant::Null));
            for (size_t c = 0; c < colNames.size(); c++) {
                auto it = rowData->m_struct->find(colNames[c]);
                if (it != rowData->m_struct->end()) {
                    values[c] = it->second;
                }
            }
            addRow(values);
        } else if (rowData->m_type == cfvariant::Array) {
            bool rowsAreStructs = false;
            if (!rowData->m_array->empty() && (*rowData->m_array)[0].m_type == cfvariant::Struct) {
                rowsAreStructs = true;
            }
            for (size_t r = 0; r < rowData->m_array->size(); r++) {
                const cfvariant &row = (*rowData->m_array)[r];
                if (rowsAreStructs && row.m_type == cfvariant::Struct) {
                    for (auto &kv : *row.m_struct) {
                        string k = kv.first;
                        k.toUpper();
                        bool found = false;
                        for (auto &cn : colNames) {
                            if (cn.equals(k)) { found = true; break; }
                        }
                        if (!found) {
                            throw webstrada::exception("QueryNew: Column '" + kv.first + "' is not in the query's column list");
                        }
                    }
                    std::vector<cfvariant> values(colNames.size(), cfvariant(cfvariant::Null));
                    for (size_t c = 0; c < colNames.size(); c++) {
                        auto it = row.m_struct->find(colNames[c]);
                        if (it != row.m_struct->end()) values[c] = it->second;
                    }
                    addRow(values);
                } else if (row.m_type == cfvariant::Array) {
                    // positional row: index i maps to colNames[i]
                    std::vector<cfvariant> values;
                    for (size_t c = 0; c < row.m_array->size(); c++) {
                        values.push_back((*row.m_array)[c]);
                    }
                    addRow(values);
                } else {
                    // A scalar element in the row array is not valid row data;
                    // CF throws (verified against CF 2021).
                    throw webstrada::exception("QueryNew: Invalid row data");
                }
            }
        }
        // any other rowData type: CF treats it as a single row of one column
        else {
            std::vector<cfvariant> values(colNames.size(), cfvariant(cfvariant::Null));
            if (!colNames.empty()) values[0] = *rowData;
            addRow(values);
        }
    }

    // Build the query payload.
    QueryData *qd = new QueryData();
    qd->m_rowCount = rowCount;
    for (size_t c = 0; c < colNames.size(); c++) {
        QueryColumn col;
        col.name = colOrigNames[c];
        col.type = colTypes[c];
        col.values = cells[c];
        qd->columns.push_back(col);
    }
    query_data_release(ret->m_query);
    ret->m_query = qd;
    return ret;
}

} // namespace cfml

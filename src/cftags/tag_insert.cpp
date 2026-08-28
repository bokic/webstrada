/**
 * @file tag_insert.cpp
 * @brief <cfinsert> / <cfupdate> runtime (cf_insert / cf_update).
 *
 * Both tags build a SQL statement from the FORM scope for a given
 * datasource/tablename and execute it. <cfinsert> builds an INSERT of the form
 * fields that match table columns; <cfupdate> builds an UPDATE keyed on the
 * table's primary key column(s), with the SET list limited to the non-key
 * form fields (or the `formfields` attribute's list). The table's columns and
 * primary key are read from the database via PRAGMA table_info. Errors
 * reproduce CF's InsertTag/UpdateTag messages.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/db.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace cfml {

namespace {

// A table column's name + declared SQL type + whether it is part of the primary
// key, read from the abstract DB layer (backend-specific introspection).
struct ColumnInfo {
    std::string name;    // original case (lookups case-insensitive)
    std::string type;    // e.g. "INTEGER", "TEXT"
    bool isPk = false;
};

std::vector<ColumnInfo> tableColumns(const std::string &dsn, const std::string &table)
{
    std::vector<ColumnInfo> cols;
    db::DBConnection *conn = db::getConnection(dsn, 0);
    for (auto &cm : conn->tableColumns(table)) {
        ColumnInfo ci;
        ci.name = cm.name;
        ci.type = cm.type;
        ci.isPk = cm.isPk;
        cols.push_back(ci);
    }
    return cols;
}

// Case-insensitive column lookup; returns index or -1.
int findColumn(const std::vector<ColumnInfo> &cols, const std::string &name)
{
    for (size_t i = 0; i < cols.size(); i++) {
        if (strcasecmp(cols[i].name.c_str(), name.c_str()) == 0) return (int)i;
    }
    return -1;
}

// The declared column type uppercased (for value coercion).
std::string upperType(const std::string &t)
{
    std::string up;
    for (char c : t) up += static_cast<char>(toupper((unsigned char)c));
    return up;
}

// Format a form value as an inline SQL literal, coercing to the column's
// declared type (integer/numeric columns unquoted, text columns quoted).
std::string formatFormValue(const std::string &value, const std::string &colType)
{
    std::string up = upperType(colType);
    if (up.find("INT") != std::string::npos) {
        // Numeric column: coerce like CF (invalid -> the fieldname error is
        // thrown before this; a numeric-looking value is emitted raw).
        const char *p = value.c_str();
        char *end = nullptr;
        double d = strtod(p, &end);
        if (end != p && *end == '\0') {
            long long iv = (long long)d;
            return std::to_string(iv);
        }
    }
    if (up.find("REAL") != std::string::npos || up.find("FLOA") != std::string::npos ||
        up.find("DOUB") != std::string::npos || up.find("NUM") != std::string::npos ||
        up.find("DEC") != std::string::npos || up.find("MONEY") != std::string::npos) {
        const char *p = value.c_str();
        char *end = nullptr;
        double d = strtod(p, &end);
        if (end != p && *end == '\0') {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.10g", d);
            return buf;
        }
    }
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "''";
        else out += c;
    }
    out += "'";
    return out;
}

// Reads the form fields to use: the `formfields` attribute's comma list when
// given, otherwise every key of the FORM scope (excluding the synthetic
// FIELDNAMES key). The FORM scope stores keys uppercased.
std::vector<std::string> formFieldNames(const cfvariant *form, const std::string &formfields)
{
    std::vector<std::string> names;
    if (!formfields.empty()) {
        size_t start = 0, pos;
        while ((pos = formfields.find(',', start)) != std::string::npos) {
            std::string f = formfields.substr(start, pos - start);
            // trim
            size_t b = 0, e = f.size();
            while (b < e && isspace((unsigned char)f[b])) b++;
            while (e > b && isspace((unsigned char)f[e - 1])) e--;
            names.push_back(f.substr(b, e - b));
            start = pos + 1;
        }
        std::string f = formfields.substr(start);
        size_t b = 0, e = f.size();
        while (b < e && isspace((unsigned char)f[b])) b++;
        while (e > b && isspace((unsigned char)f[e - 1])) e--;
        names.push_back(f.substr(b, e - b));
        return names;
    }
    if (form && form->m_type == cfvariant::Struct && form->m_struct) {
        if (form->m_structInsertOrder) {
            for (auto &k : *form->m_structInsertOrder) {
                if (k.compareCaseInsensitive("FIELDNAMES") == 0) continue;
                if (k.compareCaseInsensitive("form") == 0) continue;
                names.push_back(safe_to_std_string(k));
            }
        } else {
            for (auto &kv : *form->m_struct) {
                if (kv.first.compareCaseInsensitive("FIELDNAMES") == 0) continue;
                if (kv.first.compareCaseInsensitive("form") == 0) continue;
                names.push_back(safe_to_std_string(kv.first));
            }
        }
    }
    return names;
}

// Look up a form field value by name (case-insensitive).
std::string formFieldValue(const cfvariant *form, const std::string &name)
{
    if (!form || form->m_type != cfvariant::Struct || !form->m_struct) return "";
    string key(name.c_str());
    auto it = form->m_struct->find(key);
    if (it == form->m_struct->end()) return "";
    return safe_to_std_string(it->second);
}

// Build and run an INSERT from the form fields. Shared by cf_insert.
void runInsert(const cfvariant *attrs, void *cgi, void *server, void *cookie,
               void *application, void *session, void *url, void *form,
               void *variables)
{
    auto attr = [&](const char *key) -> const cfvariant * {
        if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
        string k(key);
        auto it = attrs->m_struct->find(k);
        return it == attrs->m_struct->end() ? nullptr : &it->second;
    };

    std::string dsn = attr("datasource") ? safe_to_std_string(*attr("datasource")) : "";
    std::string table = attr("tablename") ? safe_to_std_string(*attr("tablename")) : "";
    std::string formfields = attr("formfields") ? safe_to_std_string(*attr("formfields")) : "";

    std::vector<ColumnInfo> cols = tableColumns(dsn, table);

    // Determine which form fields to insert: the formfields list (in the given
    // order) or all form fields that are table columns.
    const cfvariant *formScope = static_cast<const cfvariant*>(form);
    std::vector<std::string> names = formFieldNames(formScope, formfields);

    std::string colList, valList;
    for (auto &name : names) {
        std::string up;
        for (char c : name) up += static_cast<char>(toupper((unsigned char)c));
        int idx = findColumn(cols, name);
        if (idx < 0) {
            // CF uppercases the field name in the message.
            webstrada::string msg("The ");
            msg.append(up.c_str());
            msg.append(" fieldname cannot be found in the ");
            msg.append(table.c_str());
            msg.append(" table.");
            throw webstrada::exception(msg);
        }
        if (!colList.empty()) colList += ",";
        colList += cols[idx].name;
        std::string value = formFieldValue(formScope, name);
        if (!valList.empty()) valList += ",";
        valList += formatFormValue(value, cols[idx].type);
    }

    std::string sql = "INSERT INTO " + table + " (" + colList + ") VALUES (" + valList + ")";

    cfvariant attrs2(cfvariant::Struct);
    attrs2.structSet(string("datasource"), cfvariant(dsn.c_str()));
    // cf_run_query already registers the returned query as a temp; we only
    // need the statement to execute (the result is discarded).
    (void)cf_run_query(sql, &attrs2, cgi, server, cookie, application, session, url, form, variables);
}

} // namespace

void cf_insert_tag(const cfvariant *attrs,
               void *cgi, void *server, void *cookie, void *application,
               void *session, void *url, void *form, void *variables)
{
    cfml::trace_record_event("DB_INSERT_START", "", "", 0);
    runInsert(attrs, cgi, server, cookie, application, session, url, form, variables);
    cfml::trace_record_event("DB_INSERT_END", "", "", 0);
}

void cf_update(const cfvariant *attrs,
               void *cgi, void *server, void *cookie, void *application,
               void *session, void *url, void *form, void *variables)
{
    cfml::trace_record_event("DB_UPDATE_START", "", "", 0);
    auto attr = [&](const char *key) -> const cfvariant * {
        if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
        string k(key);
        auto it = attrs->m_struct->find(k);
        return it == attrs->m_struct->end() ? nullptr : &it->second;
    };

    std::string dsn = attr("datasource") ? safe_to_std_string(*attr("datasource")) : "";
    std::string table = attr("tablename") ? safe_to_std_string(*attr("tablename")) : "";
    std::string formfields = attr("formfields") ? safe_to_std_string(*attr("formfields")) : "";

    std::vector<ColumnInfo> cols = tableColumns(dsn, table);
    const cfvariant *formScope = static_cast<const cfvariant*>(form);

    // Find the primary key columns.
    std::vector<int> pkIdx;
    for (size_t i = 0; i < cols.size(); i++) {
        if (cols[i].isPk) pkIdx.push_back((int)i);
    }

    // The fields to update: the formfields list when given, otherwise ALL form
    // fields. CF validates every one of them against the table columns first
    // (a non-column field throws "The X fieldname cannot be found in the Y
    // table."), then requires the primary-key fields to be present in the form.
    std::vector<std::string> names = formFieldNames(formScope, formfields);

    // 1. Field validation: every field to update must be a table column.
    for (auto &name : names) {
        if (findColumn(cols, name) < 0) {
            std::string up;
            for (char c : name) up += static_cast<char>(toupper((unsigned char)c));
            webstrada::string msg("The ");
            msg.append(up.c_str());
            msg.append(" fieldname cannot be found in the ");
            msg.append(table.c_str());
            msg.append(" table.");
            throw webstrada::exception(msg);
        }
    }

    // 2. Every primary-key field must be present in the form (an empty value is
    // accepted; a missing one throws CF's message).
    std::vector<std::string> pkValues;
    for (int idx : pkIdx) {
        std::string up;
        for (char c : cols[idx].name) up += static_cast<char>(toupper((unsigned char)c));
        std::string v = formFieldValue(formScope, cols[idx].name);
        if (v.empty() && formFieldValue(formScope, up).empty()) {
            webstrada::string msg("Primary key field ");
            msg.append(up.c_str());
            msg.append(", not found. The field ");
            msg.append(up.c_str());
            msg.append(", was not found in the form input. This field is required to do an update because it is part of the primary key for the ");
            msg.append(table.c_str());
            msg.append(" table.");
            throw webstrada::exception(msg);
        }
        pkValues.push_back(v.empty() ? formFieldValue(formScope, up) : v);
    }

    // 3. Build the SET list (excluding primary-key columns, which key the
    // WHERE) and the WHERE clause.
    std::string setList;
    for (auto &name : names) {
        int idx = findColumn(cols, name);
        // Skip primary-key columns in the SET list (they key the WHERE).
        bool isPk = false;
        for (int p : pkIdx) if (p == idx) isPk = true;
        if (isPk) continue;
        if (!setList.empty()) setList += ",";
        setList += cols[idx].name + " = " + formatFormValue(formFieldValue(formScope, name), cols[idx].type);
    }

    std::string where;
    for (size_t i = 0; i < pkIdx.size(); i++) {
        int idx = pkIdx[i];
        if (!where.empty()) where += " AND ";
        where += cols[idx].name + " = " + formatFormValue(pkValues[i], cols[idx].type);
    }

    std::string sql = "UPDATE " + table + " SET " + setList;
    if (!where.empty()) sql += " WHERE " + where;

    cfvariant attrs2(cfvariant::Struct);
    attrs2.structSet(string("datasource"), cfvariant(dsn.c_str()));
    // cf_run_query already registers the returned query as a temp; we only
    // need the statement to execute (the result is discarded).
    (void)cf_run_query(sql, &attrs2, cgi, server, cookie, application, session, url, form, variables);
    cfml::trace_record_event("DB_UPDATE_END", dsn.c_str(), table.c_str(), 0);
}

} // namespace cfml

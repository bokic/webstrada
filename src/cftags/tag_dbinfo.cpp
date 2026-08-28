/**
 * @file tag_dbinfo.cpp
 * @brief <cfdbinfo> runtime (cf_dbinfo).
 *
 * Retrieves datasource metadata and stores the result query in the `name`
 * attribute. Supported types: tables, columns, version, procedures,
 * foreignkeys, index, dbnames. The metadata is read through the abstract DB
 * layer, with backend-specific introspection for each supported driver:
 * SQLite (sqlite_master / PRAGMA), MySQL/MariaDB and PostgreSQL
 * (INFORMATION_SCHEMA / pg_catalog). Column sets and value formats reproduce
 * CF's SQLite driver output (verified against CF 2025 on the RDS host); the
 * MySQL/PostgreSQL values follow those engines' catalogs and CF's documented
 * column sets. Attribute validation (missing name/table, invalid type)
 * reproduces CF's CFDBINFO messages.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/db.h>

#include <sqlite3.h>
#ifdef WEBSTRADA_HAVE_MYSQL
#include <mysql.h>
#endif
#ifdef WEBSTRADA_HAVE_POSTGRES
#include <libpq-fe.h>
#endif

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace cfml {

namespace {

struct DbInfoCol {
    std::string name;    // output column name (uppercased, like CF)
    std::vector<cfvariant> values;
};

struct DbInfoResult {
    std::vector<DbInfoCol> cols;
    long long rowCount = 0;
};

// Run a metadata SQL and capture its rows as a DbInfoResult (columns named by
// the AS aliases, uppercased like CF's driver).
DbInfoResult runMeta(const std::string &dsn, const std::string &sql)
{
    DbInfoResult out;
    db::DBConnection *conn = db::getConnection(dsn, 0);
    db::DBResult r = conn->execute(sql, -1);
    for (size_t i = 0; i < r.columns.size(); i++) {
        DbInfoCol c;
        std::string n = r.columns[i].name;
        for (auto &ch : n) ch = static_cast<char>(toupper((unsigned char)ch));
        c.name = n;
        c.values = std::move(r.columns[i].values);
        out.cols.push_back(std::move(c));
    }
    out.rowCount = r.rowCount;
    return out;
}

// Stringify a result cell cleanly: NULL -> "", integers without a ".0"
// suffix (a MySQL DECIMAL metadata value such as 10.0 must render as "10"),
// everything else via the variant's string conversion.
std::string cellToString(const cfvariant &v)
{
    switch (v.m_type) {
    case cfvariant::Null:
        return "";
    case cfvariant::Number:
        return std::to_string(v.m_int);
    case cfvariant::Long:
        return std::to_string(v.m_long);
    case cfvariant::Float: {
        double d = v.m_double;
        if (d == static_cast<long long>(d) && d >= -9.0e18 && d <= 9.0e18)
            return std::to_string(static_cast<long long>(d));
        return safe_to_std_string(v);
    }
    default:
        return safe_to_std_string(v);
    }
}

// Run a metadata query and return the first cell of the first column as a
// string ("" when the result has no rows), used by the VERSION type.
std::string runScalar(const std::string &dsn, const std::string &sql)
{
    db::DBConnection *conn = db::getConnection(dsn, 0);
    std::string out;
    db::DBResult r = conn->execute(sql, -1);
    if (!r.columns.empty() && !r.columns[0].values.empty()) {
        out = cellToString(r.columns[0].values[0]);
    }
    return out;
}

// SQL-escape a string literal for a metadata query.
std::string sqlQuote(const std::string &s)
{
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "''";
        else out += c;
    }
    out += "'";
    return out;
}

// Normalized backend name for a datasource: "sqlite" (the default), "mysql" or
// "postgres" (the "postgresql"/"pg" aliases are folded).
std::string dsnBackend(const std::string &dsn)
{
    std::string b = webstrada::db::datasourceConfig(dsn).backend;
    for (auto &c : b) c = static_cast<char>(tolower((unsigned char)c));
    if (b == "postgresql" || b == "pg") b = "postgres";
    if (b.empty()) b = "sqlite";
    return b;
}

bool backendIsSqlite(const std::string &b) { return b == "sqlite"; }
bool backendIsPostgres(const std::string &b) { return b == "postgres"; }

// Read a result cell as a string by column index ("" when out of range).
std::string cellStr(const db::DBResult &r, int colIdx, long long row)
{
    if (colIdx < 0 || (size_t)colIdx >= r.columns.size() ||
        (size_t)row >= r.columns[colIdx].values.size())
        return "";
    return cellToString(r.columns[colIdx].values[row]);
}

// Find a result column index by its lowercase name (-1 when absent).
int findColIdx(const db::DBResult &r, const char *lname)
{
    for (size_t i = 0; i < r.columns.size(); i++) {
        std::string cn;
        for (char c : r.columns[i].name) cn += static_cast<char>(tolower((unsigned char)c));
        if (cn == lname) return (int)i;
    }
    return -1;
}

// Build a query result from a DbInfoResult into the `name` variable (only when
// the query has columns).
void storeResult(void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables,
                 const std::string &name, const DbInfoResult &res)
{
    if (name.empty()) return;
    cfvariant queryVal(cfvariant::Query);
    QueryData *qd = queryVal.m_query;
    for (auto &c : res.cols) {
        QueryColumn col;
        col.name = c.name.c_str();
        col.type = "varchar";
        col.values = c.values;
        qd->columns.push_back(std::move(col));
    }
    qd->m_rowCount = static_cast<int>(res.rowCount);
    cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                     static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                     static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                     static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                     name.c_str(), &queryVal);
}

// ---- TABLES ---------------------------------------------------------------

DbInfoResult dbinfoTables(const std::string &backend, const std::string &dsn,
                          const std::string &pattern)
{
    if (backendIsSqlite(backend)) {
        // CF's driver lists every object in sqlite_master (tables + views +
        // autoindexes) with a TABLE_NAME/TABLE_TYPE/REMARKS column set. The
        // pattern filters by name; the type is reported uppercased.
        std::string sql = "SELECT name AS TABLE_NAME, type AS TABLE_TYPE, '' AS REMARKS FROM sqlite_master";
        if (!pattern.empty()) {
            sql += " WHERE name LIKE " + sqlQuote(pattern);
        }
        DbInfoResult res = runMeta(dsn, sql);
        for (auto &col : res.cols) {
            if (col.name == "TABLE_TYPE") {
                for (auto &v : col.values) {
                    if (v.m_type == cfvariant::String && v.m_str) {
                        std::string t = safe_to_std_string(v);
                        for (auto &c : t) c = static_cast<char>(toupper((unsigned char)c));
                        v = cfvariant(t.c_str());
                    }
                }
            }
        }
        return res;
    }

    std::string sql;
    if (backendIsPostgres(backend)) {
        sql =
            "SELECT c.relname AS TABLE_NAME, "
            "CASE c.relkind WHEN 'r' THEN 'TABLE' WHEN 'p' THEN 'PARTITIONED TABLE' "
            "WHEN 'v' THEN 'VIEW' WHEN 'm' THEN 'MATERIALIZED VIEW' WHEN 'S' THEN 'SEQUENCE' "
            "WHEN 'f' THEN 'FOREIGN TABLE' WHEN 'c' THEN 'COMPOSITE TYPE' ELSE 'OTHER' END AS TABLE_TYPE, "
            "COALESCE(obj_description(c.oid, 'pg_class'), '') AS REMARKS "
            "FROM pg_catalog.pg_class c JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
            "WHERE n.nspname = current_schema() AND c.relkind IN ('r','p','v','m','S','f','c')";
        if (!pattern.empty()) {
            sql += " AND c.relname LIKE " + sqlQuote(pattern);
        }
        sql += " ORDER BY c.relname";
    } else {
        sql =
            "SELECT TABLE_NAME AS TABLE_NAME, TABLE_TYPE AS TABLE_TYPE, "
            "COALESCE(TABLE_COMMENT, '') AS REMARKS FROM INFORMATION_SCHEMA.TABLES "
            "WHERE TABLE_SCHEMA = DATABASE()";
        if (!pattern.empty()) {
            sql += " AND TABLE_NAME LIKE " + sqlQuote(pattern);
        }
        sql += " ORDER BY TABLE_NAME";
    }
    DbInfoResult res = runMeta(dsn, sql);
    if (!backendIsPostgres(backend)) {
        // CF's MySQL driver reports base tables as "TABLE" (Connector/J strips
        // the "BASE " prefix of information_schema's "BASE TABLE").
        for (auto &col : res.cols) {
            if (col.name == "TABLE_TYPE") {
                for (auto &v : col.values) {
                    if (v.m_type == cfvariant::String && v.m_str) {
                        std::string t = safe_to_std_string(v);
                        if (t == "BASE TABLE") t = "TABLE";
                        v = cfvariant(t.c_str());
                    }
                }
            }
        }
    }
    return res;
}

// ---- COLUMNS --------------------------------------------------------------

// CF's column order (verified against the RDS host's SQLite driver; the same
// set is returned for every backend).
const char *const kColumnsCols[] = {
    "TABLE_NAME", "COLUMN_NAME", "TYPE_NAME", "COLUMN_SIZE",
    "DECIMAL_DIGITS", "IS_NULLABLE", "IS_PRIMARYKEY", "IS_FOREIGNKEY",
    "ORDINAL_POSITION", "COLUMN_DEFAULT_VALUE", "CHAR_OCTET_LENGTH",
    "REMARKS", "REFERENCED_PRIMARYKEY", "REFERENCED_PRIMARYKEY_TABLE"
};

DbInfoResult dbinfoColumns(const std::string &backend, const std::string &dsn,
                           const std::string &table)
{
    std::string sql;
    bool sqlite = backendIsSqlite(backend);
    if (sqlite) {
        sql = "PRAGMA table_info(" + table + ")";
    } else if (backendIsPostgres(backend)) {
        // information_schema.columns joined with pg_type for the JDBC-style
        // type name (pgjdbc reports the pg_type typname, e.g. int4/varchar) and
        // the PRIMARY KEY / FOREIGN KEY constraints.
        sql =
            "SELECT c.COLUMN_NAME AS COL, t.typname AS TYPE_NAME, "
            "COALESCE(c.CHARACTER_MAXIMUM_LENGTH, c.NUMERIC_PRECISION, 0) AS COL_SIZE, "
            "COALESCE(c.NUMERIC_SCALE, 0) AS DEC_DIGITS, "
            "c.IS_NULLABLE AS IS_NULLABLE, c.ORDINAL_POSITION AS ORD, "
            "c.COLUMN_DEFAULT AS DEFVAL, COALESCE(c.CHARACTER_OCTET_LENGTH, 0) AS CHAR_OCTET, "
            "'' AS REMARKS, "
            "CASE WHEN pk.COLUMN_NAME IS NOT NULL THEN 'YES' ELSE 'NO' END AS IS_PK, "
            "CASE WHEN fk.COLUMN_NAME IS NOT NULL THEN 'YES' ELSE 'NO' END AS IS_FK, "
            "COALESCE(fkref.COLUMN_NAME, '') AS REF_PK, "
            "COALESCE(fkref.TABLE_NAME, '') AS REF_PK_TABLE "
            "FROM information_schema.columns c "
            "LEFT JOIN pg_catalog.pg_attribute a "
            "  ON a.attrelid = format('%I.%I', c.table_schema, c.table_name)::regclass "
            " AND a.attname = c.column_name "
            "LEFT JOIN pg_catalog.pg_type t ON t.oid = a.atttypid "
            "LEFT JOIN information_schema.table_constraints tc "
            "  ON tc.table_schema = c.table_schema AND tc.table_name = c.table_name "
            " AND tc.constraint_type = 'PRIMARY KEY' "
            "LEFT JOIN information_schema.key_column_usage pk "
            "  ON pk.constraint_name = tc.constraint_name AND pk.table_schema = tc.table_schema "
            " AND pk.column_name = c.column_name "
            "LEFT JOIN information_schema.table_constraints ftc "
            "  ON ftc.table_schema = c.table_schema AND ftc.table_name = c.table_name "
            " AND ftc.constraint_type = 'FOREIGN KEY' "
            "LEFT JOIN information_schema.key_column_usage fk "
            "  ON fk.constraint_schema = ftc.constraint_schema "
            " AND fk.constraint_name = ftc.constraint_name AND fk.column_name = c.column_name "
            "LEFT JOIN information_schema.constraint_column_usage fkref "
            "  ON fkref.constraint_schema = ftc.constraint_schema "
            " AND fkref.constraint_name = ftc.constraint_name "
            " AND fk.COLUMN_NAME IS NOT NULL "
            "WHERE c.table_schema = current_schema() AND c.table_name = " + sqlQuote(table) +
            " ORDER BY c.ordinal_position";
    } else {
        // MySQL/MariaDB information_schema.columns, with the primary-key usage
        // and the foreign-key reference resolved in one pass.
        sql =
            "SELECT c.COLUMN_NAME AS COL, UPPER(c.DATA_TYPE) AS TYPE_NAME, "
            "COALESCE(c.CHARACTER_MAXIMUM_LENGTH, c.NUMERIC_PRECISION, 0) AS COL_SIZE, "
            "COALESCE(c.NUMERIC_SCALE, 0) AS DEC_DIGITS, "
            "c.IS_NULLABLE AS IS_NULLABLE, c.ORDINAL_POSITION AS ORD, "
            "c.COLUMN_DEFAULT AS DEFVAL, COALESCE(c.CHARACTER_OCTET_LENGTH, 0) AS CHAR_OCTET, "
            "COALESCE(c.COLUMN_COMMENT, '') AS REMARKS, "
            "CASE WHEN pk.COLUMN_NAME IS NOT NULL THEN 'YES' ELSE 'NO' END AS IS_PK, "
            "CASE WHEN fk.COLUMN_NAME IS NOT NULL THEN 'YES' ELSE 'NO' END AS IS_FK, "
            "COALESCE(fk.REFERENCED_COLUMN_NAME, '') AS REF_PK, "
            "COALESCE(fk.REFERENCED_TABLE_NAME, '') AS REF_PK_TABLE "
            "FROM INFORMATION_SCHEMA.COLUMNS c "
            "LEFT JOIN (SELECT k.TABLE_SCHEMA, k.TABLE_NAME, k.COLUMN_NAME "
            "           FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE k "
            "           WHERE k.CONSTRAINT_NAME = 'PRIMARY') pk "
            "  ON pk.TABLE_SCHEMA = c.TABLE_SCHEMA AND pk.TABLE_NAME = c.TABLE_NAME "
            " AND pk.COLUMN_NAME = c.COLUMN_NAME "
            "LEFT JOIN (SELECT k.TABLE_SCHEMA, k.TABLE_NAME, k.COLUMN_NAME, "
            "                  k.REFERENCED_COLUMN_NAME, k.REFERENCED_TABLE_NAME "
            "           FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE k "
            "           WHERE k.REFERENCED_TABLE_NAME IS NOT NULL) fk "
            "  ON fk.TABLE_SCHEMA = c.TABLE_SCHEMA AND fk.TABLE_NAME = c.TABLE_NAME "
            " AND fk.COLUMN_NAME = c.COLUMN_NAME "
            "WHERE c.TABLE_SCHEMA = DATABASE() AND c.TABLE_NAME = " + sqlQuote(table) +
            " ORDER BY c.ORDINAL_POSITION";
    }

    db::DBConnection *conn = db::getConnection(dsn, 0);
    DbInfoResult res;
    db::DBResult r = conn->execute(sql, -1);

        if (sqlite) {
            // Map the PRAGMA rows (cid,name,type,notnull,dflt_value,pk) to CF's
            // COLUMNS column set (verified byte-for-byte against CF 2025).
            int nameIdx = -1, typeIdx = -1, pkIdx = -1, notnullIdx = -1, dfltIdx = -1;
            for (size_t i = 0; i < r.columns.size(); i++) {
                std::string cn;
                for (char c : r.columns[i].name) cn += static_cast<char>(tolower((unsigned char)c));
                if (cn == "name") nameIdx = (int)i;
                else if (cn == "type") typeIdx = (int)i;
                else if (cn == "pk") pkIdx = (int)i;
                else if (cn == "notnull") notnullIdx = (int)i;
                else if (cn == "dflt_value") dfltIdx = (int)i;
            }
            for (int ci = 0; ci < 14; ci++) {
                DbInfoCol c;
                c.name = kColumnsCols[ci];
                for (long long row = 0; row < r.rowCount; row++) {
                    std::string cell = "";
                    switch (ci) {
                    case 0: cell = table; break;
                    case 1: cell = (nameIdx >= 0) ? safe_to_std_string(r.columns[nameIdx].values[row]) : ""; break;
                    case 2: cell = (typeIdx >= 0) ? safe_to_std_string(r.columns[typeIdx].values[row]) : ""; break;
                    case 3: // COLUMN_SIZE: 2000000000 like CF's driver
                        cell = "2000000000"; break;
                    case 4: // DECIMAL_DIGITS: 0
                        cell = "0"; break;
                    case 5: // IS_NULLABLE: YES (SQLite has no NOT NULL default)
                        cell = (notnullIdx >= 0 && r.columns[notnullIdx].values[row].m_type == cfvariant::Number &&
                                r.columns[notnullIdx].values[row].m_int != 0) ? "NO" : "YES";
                        break;
                    case 6: // IS_PRIMARYKEY
                        cell = (pkIdx >= 0 && r.columns[pkIdx].values[row].m_type == cfvariant::Number &&
                                r.columns[pkIdx].values[row].m_int > 0) ? "YES" : "NO";
                        break;
                    case 7: // IS_FOREIGNKEY: NO (SQLite exposes FKs separately)
                        cell = "NO"; break;
                    case 8: // ORDINAL_POSITION: 1-based
                        cell = std::to_string(row + 1); break;
                    case 9: // COLUMN_DEFAULT_VALUE
                        cell = (dfltIdx >= 0) ? safe_to_std_string(r.columns[dfltIdx].values[row]) : "";
                        break;
                    case 10: // CHAR_OCTET_LENGTH: 2000000000
                        cell = "2000000000"; break;
                    default:
                        cell = ""; break;
                    }
                    c.values.push_back(cfvariant(cell.c_str()));
                }
                res.cols.push_back(std::move(c));
            }
        } else {
            int colIdx = findColIdx(r, "col");
            int typeIdx = findColIdx(r, "type_name");
            int sizeIdx = findColIdx(r, "col_size");
            int decIdx = findColIdx(r, "dec_digits");
            int nullIdx = findColIdx(r, "is_nullable");
            int ordIdx = findColIdx(r, "ord");
            int defIdx = findColIdx(r, "defval");
            int octetIdx = findColIdx(r, "char_octet");
            int remIdx = findColIdx(r, "remarks");
            int pkIdx = findColIdx(r, "is_pk");
            int fkIdx = findColIdx(r, "is_fk");
            int refPkIdx = findColIdx(r, "ref_pk");
            int refPkTableIdx = findColIdx(r, "ref_pk_table");
            for (int ci = 0; ci < 14; ci++) {
                DbInfoCol c;
                c.name = kColumnsCols[ci];
                for (long long row = 0; row < r.rowCount; row++) {
                    std::string cell;
                    switch (ci) {
                    case 0: cell = table; break;
                    case 1: cell = cellStr(r, colIdx, row); break;
                    case 2: cell = cellStr(r, typeIdx, row); break;
                    case 3: cell = cellStr(r, sizeIdx, row); break;
                    case 4: cell = cellStr(r, decIdx, row); break;
                    case 5: cell = cellStr(r, nullIdx, row); break;
                    case 6: cell = cellStr(r, pkIdx, row); break;
                    case 7: cell = cellStr(r, fkIdx, row); break;
                    case 8: cell = cellStr(r, ordIdx, row); break;
                    case 9: cell = cellStr(r, defIdx, row); break;
                    case 10: cell = cellStr(r, octetIdx, row); break;
                    case 11: cell = cellStr(r, remIdx, row); break;
                    case 12: cell = cellStr(r, refPkIdx, row); break;
                    case 13: cell = cellStr(r, refPkTableIdx, row); break;
                    }
                    c.values.push_back(cfvariant(cell.c_str()));
                }
                res.cols.push_back(std::move(c));
            }
        }
        res.rowCount = r.rowCount;
    return res;
}

// ---- VERSION --------------------------------------------------------------

#ifdef WEBSTRADA_HAVE_POSTGRES
std::string pqDriverVersion()
{
    int v = PQlibVersion();
    return std::to_string(v / 10000) + "." + std::to_string((v / 100) % 100) + "." +
           std::to_string(v % 100);
}
#endif

DbInfoResult dbinfoVersion(const std::string &backend, const std::string &dsn)
{
    DbInfoResult res;
    if (backendIsSqlite(backend)) {
        std::string sqliteVersion = sqlite3_libversion();
        DbInfoCol names[] = {
            {"DATABASE_PRODUCTNAME", {cfvariant("SQLite")}},
            {"DATABASE_VERSION", {cfvariant(sqliteVersion.c_str())}},
            {"DRIVER_NAME", {cfvariant("WebStrada SQLite")}},
            {"DRIVER_VERSION", {cfvariant(sqliteVersion.c_str())}},
        };
        for (auto &c : names) res.cols.push_back(c);
        res.rowCount = 1;
        return res;
    }

    std::string product, driverName, driverVersion;
    if (backendIsPostgres(backend)) {
        product = "PostgreSQL";
        driverName = "WebStrada PostgreSQL";
#ifdef WEBSTRADA_HAVE_POSTGRES
        driverVersion = pqDriverVersion();
#endif
        DbInfoCol names[] = {
            {"DATABASE_PRODUCTNAME", {cfvariant(product.c_str())}},
            {"DATABASE_VERSION", {cfvariant(runScalar(dsn, "SHOW server_version").c_str())}},
            {"DRIVER_NAME", {cfvariant(driverName.c_str())}},
            {"DRIVER_VERSION", {cfvariant(driverVersion.c_str())}},
        };
        for (auto &c : names) res.cols.push_back(c);
        res.rowCount = 1;
        return res;
    }

    product = "MySQL";
    driverName = "WebStrada MySQL";
#ifdef WEBSTRADA_HAVE_MYSQL
    driverVersion = mysql_get_client_info();
#endif
    DbInfoCol names[] = {
        {"DATABASE_PRODUCTNAME", {cfvariant(product.c_str())}},
        {"DATABASE_VERSION", {cfvariant(runScalar(dsn, "SELECT VERSION()").c_str())}},
        {"DRIVER_NAME", {cfvariant(driverName.c_str())}},
        {"DRIVER_VERSION", {cfvariant(driverVersion.c_str())}},
    };
    for (auto &c : names) res.cols.push_back(c);
    res.rowCount = 1;
    return res;
}

// ---- PROCEDURES -----------------------------------------------------------

DbInfoResult dbinfoProcedures(const std::string &backend, const std::string &dsn)
{
    if (backendIsSqlite(backend)) {
        // SQLite has no stored procedures; empty result with CF's column set.
        static const char *const kCols[] = {"PROCEDURE_NAME", "PROCEDURE_TYPE", "REMARKS"};
        DbInfoResult res;
        for (const char *cn : kCols) {
            DbInfoCol c;
            c.name = cn;
            res.cols.push_back(std::move(c));
        }
        res.rowCount = 0;
        return res;
    }

    std::string sql;
    if (backendIsPostgres(backend)) {
        // PostgreSQL's information_schema.routines has no ROUTINE_COMMENT
        // column; report an empty REMARKS.
        sql = "SELECT ROUTINE_NAME AS PROCEDURE_NAME, ROUTINE_TYPE AS PROCEDURE_TYPE, "
              "'' AS REMARKS FROM INFORMATION_SCHEMA.ROUTINES "
              "WHERE ROUTINE_SCHEMA = current_schema() ORDER BY ROUTINE_NAME";
    } else {
        sql = "SELECT ROUTINE_NAME AS PROCEDURE_NAME, ROUTINE_TYPE AS PROCEDURE_TYPE, "
              "COALESCE(ROUTINE_COMMENT, '') AS REMARKS FROM INFORMATION_SCHEMA.ROUTINES "
              "WHERE ROUTINE_SCHEMA = DATABASE() ORDER BY ROUTINE_NAME";
    }
    return runMeta(dsn, sql);
}

// ---- DBNAMES --------------------------------------------------------------

DbInfoResult dbinfoDbNames(const std::string &backend, const std::string &dsn)
{
    if (backendIsSqlite(backend)) {
        // SQLite is a single-file database; no db list. Empty result with CF's
        // column set.
        static const char *const kCols[] = {"DATABASE_NAME", "TYPE"};
        DbInfoResult res;
        for (const char *cn : kCols) {
            DbInfoCol c;
            c.name = cn;
            res.cols.push_back(std::move(c));
        }
        res.rowCount = 0;
        return res;
    }

    std::string sql;
    if (backendIsPostgres(backend)) {
        sql = "SELECT datname AS DATABASE_NAME, 'database' AS TYPE "
              "FROM pg_catalog.pg_database ORDER BY datname";
    } else {
        sql = "SELECT SCHEMA_NAME AS DATABASE_NAME, 'database' AS TYPE "
              "FROM INFORMATION_SCHEMA.SCHEMATA ORDER BY SCHEMA_NAME";
    }
    return runMeta(dsn, sql);
}

// ---- FOREIGNKEYS ----------------------------------------------------------

DbInfoResult dbinfoForeignKeys(const std::string &backend, const std::string &dsn,
                               const std::string &table)
{
    static const char *const kCols[] = {
        "FKTABLE_NAME", "FKCOLUMN_NAME", "PKCOLUMN_NAME", "UPDATE_RULE", "DELETE_RULE"
    };
    std::string sql;
    if (backendIsSqlite(backend)) {
        sql = "PRAGMA foreign_key_list(" + table + ")";
    } else if (backendIsPostgres(backend)) {
        sql =
            "SELECT tc.table_name AS FKTABLE_NAME, kcu.column_name AS FKCOLUMN_NAME, "
            "ccu.column_name AS PKCOLUMN_NAME, rc.update_rule AS UPDATE_RULE, "
            "rc.delete_rule AS DELETE_RULE "
            "FROM information_schema.table_constraints tc "
            "JOIN information_schema.key_column_usage kcu "
            "  ON tc.constraint_name = kcu.constraint_name "
            " AND tc.constraint_schema = kcu.constraint_schema "
            "JOIN information_schema.referential_constraints rc "
            "  ON rc.constraint_name = tc.constraint_name "
            " AND rc.constraint_schema = tc.constraint_schema "
            "JOIN information_schema.constraint_column_usage ccu "
            "  ON ccu.constraint_name = rc.unique_constraint_name "
            " AND ccu.constraint_schema = rc.unique_constraint_schema "
            "WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_schema = current_schema() "
            "AND tc.table_name = " + sqlQuote(table);
    } else {
        sql =
            "SELECT tc.TABLE_NAME AS FKTABLE_NAME, kcu.COLUMN_NAME AS FKCOLUMN_NAME, "
            "ccu.COLUMN_NAME AS PKCOLUMN_NAME, rc.UPDATE_RULE AS UPDATE_RULE, "
            "rc.DELETE_RULE AS DELETE_RULE "
            "FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS tc "
            "JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu "
            "  ON tc.CONSTRAINT_SCHEMA = kcu.CONSTRAINT_SCHEMA "
            " AND tc.CONSTRAINT_NAME = kcu.CONSTRAINT_NAME AND tc.TABLE_NAME = kcu.TABLE_NAME "
            "JOIN INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS rc "
            "  ON rc.CONSTRAINT_SCHEMA = tc.CONSTRAINT_SCHEMA "
            " AND rc.CONSTRAINT_NAME = tc.CONSTRAINT_NAME "
            "JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE ccu "
            "  ON ccu.CONSTRAINT_SCHEMA = rc.UNIQUE_CONSTRAINT_SCHEMA "
            " AND ccu.CONSTRAINT_NAME = rc.UNIQUE_CONSTRAINT_NAME "
            " AND ccu.TABLE_NAME = rc.REFERENCED_TABLE_NAME "
            " AND ccu.ORDINAL_POSITION = kcu.ORDINAL_POSITION "
            "WHERE tc.CONSTRAINT_TYPE = 'FOREIGN KEY' AND tc.TABLE_SCHEMA = DATABASE() "
            "AND tc.TABLE_NAME = " + sqlQuote(table);
    }

    db::DBConnection *conn = db::getConnection(dsn, 0);
    DbInfoResult res;
    db::DBResult r = conn->execute(sql, -1);
    if (backendIsSqlite(backend)) {
        int fkNameIdx = -1, fkTableIdx = -1, fkFromIdx = -1, fkToIdx = -1,
            fkUpdateIdx = -1, fkDeleteIdx = -1;
        for (size_t i = 0; i < r.columns.size(); i++) {
            std::string cn;
            for (char c : r.columns[i].name) cn += static_cast<char>(tolower((unsigned char)c));
            if (cn == "id") fkNameIdx = (int)i;
            else if (cn == "table") fkTableIdx = (int)i;
            else if (cn == "from") fkFromIdx = (int)i;
            else if (cn == "to") fkToIdx = (int)i;
            else if (cn == "on_update") fkUpdateIdx = (int)i;
            else if (cn == "on_delete") fkDeleteIdx = (int)i;
        }
        for (int ci = 0; ci < 5; ci++) {
            DbInfoCol c;
            c.name = kCols[ci];
            for (long long row = 0; row < r.rowCount; row++) {
                std::string cell;
                switch (ci) {
                case 0: cell = table; break; // FKTABLE_NAME = the table itself
                case 1: cell = (fkFromIdx >= 0) ? safe_to_std_string(r.columns[fkFromIdx].values[row]) : ""; break;
                case 2: cell = (fkToIdx >= 0) ? safe_to_std_string(r.columns[fkToIdx].values[row]) : ""; break;
                case 3: cell = (fkUpdateIdx >= 0) ? safe_to_std_string(r.columns[fkUpdateIdx].values[row]) : ""; break;
                case 4: cell = (fkDeleteIdx >= 0) ? safe_to_std_string(r.columns[fkDeleteIdx].values[row]) : ""; break;
                }
                c.values.push_back(cfvariant(cell.c_str()));
            }
            res.cols.push_back(std::move(c));
        }
    } else {
        int fkTableIdx = findColIdx(r, "fktable_name");
        int fkFromIdx = findColIdx(r, "fkcolumn_name");
        int fkToIdx = findColIdx(r, "pkcolumn_name");
        int fkUpdateIdx = findColIdx(r, "update_rule");
        int fkDeleteIdx = findColIdx(r, "delete_rule");
        for (int ci = 0; ci < 5; ci++) {
            DbInfoCol c;
            c.name = kCols[ci];
            for (long long row = 0; row < r.rowCount; row++) {
                std::string cell;
                switch (ci) {
                case 0: cell = cellStr(r, fkTableIdx, row); break;
                case 1: cell = cellStr(r, fkFromIdx, row); break;
                case 2: cell = cellStr(r, fkToIdx, row); break;
                case 3: cell = cellStr(r, fkUpdateIdx, row); break;
                case 4: cell = cellStr(r, fkDeleteIdx, row); break;
                }
                c.values.push_back(cfvariant(cell.c_str()));
            }
            res.cols.push_back(std::move(c));
        }
    }
    res.rowCount = r.rowCount;
    return res;
}

// ---- INDEX ----------------------------------------------------------------

DbInfoResult dbinfoIndex(const std::string &backend, const std::string &dsn,
                         const std::string &table)
{
    static const char *const kCols[] = {
        "INDEX_NAME", "COLUMN_NAME", "ORDINAL_POSITION", "NON_UNIQUE", "TYPE", "CARDINALITY", "PAGES"
    };

    if (backendIsSqlite(backend)) {
        // PRAGMA index_list(table) + index_info(index) -> CF's INDEX columns.
        db::DBConnection *conn = db::getConnection(dsn, 0);
        DbInfoResult res;
        db::DBResult list = conn->execute("PRAGMA index_list(" + table + ")", -1);
        int seqIdx = -1, nameIdx = -1, uniqueIdx = -1;
        for (size_t i = 0; i < list.columns.size(); i++) {
            std::string cn;
            for (char c : list.columns[i].name) cn += static_cast<char>(tolower((unsigned char)c));
            if (cn == "seq") seqIdx = (int)i;
            else if (cn == "name") nameIdx = (int)i;
            else if (cn == "unique") uniqueIdx = (int)i;
        }
        std::vector<cfvariant> colVals[7];
        for (long long li = 0; li < list.rowCount; li++) {
            std::string idxName = (nameIdx >= 0) ? safe_to_std_string(list.columns[nameIdx].values[li]) : "";
            bool isUnique = (uniqueIdx >= 0 && list.columns[uniqueIdx].values[li].m_type == cfvariant::Number &&
                             list.columns[uniqueIdx].values[li].m_int != 0);
            db::DBResult info = conn->execute("PRAGMA index_info(" + sqlQuote(idxName) + ")", -1);
            int cseqIdx = -1, cnameIdx = -1;
            for (size_t i = 0; i < info.columns.size(); i++) {
                std::string cn;
                for (char c : info.columns[i].name) cn += static_cast<char>(tolower((unsigned char)c));
                if (cn == "seqno") cseqIdx = (int)i;
                else if (cn == "name") cnameIdx = (int)i;
            }
            for (long long ri = 0; ri < info.rowCount; ri++) {
                std::string colName = (cnameIdx >= 0) ? safe_to_std_string(info.columns[cnameIdx].values[ri]) : "";
                long long ord = (cseqIdx >= 0 && info.columns[cseqIdx].values[ri].m_type == cfvariant::Number)
                    ? info.columns[cseqIdx].values[ri].m_int + 1 : 1;
                colVals[0].push_back(cfvariant(idxName.c_str()));
                colVals[1].push_back(cfvariant(colName.c_str()));
                colVals[2].push_back(cfvariant(static_cast<int>(ord)));
                colVals[3].push_back(cfvariant(isUnique ? "NO" : "YES"));
                colVals[4].push_back(cfvariant("Other Index"));
                colVals[5].push_back(cfvariant("0")); // cardinality
                colVals[6].push_back(cfvariant("0")); // pages
            }
        }
        res.rowCount = (long long)colVals[0].size();
        for (int ci = 0; ci < 7; ci++) {
            DbInfoCol c;
            c.name = kCols[ci];
            c.values = std::move(colVals[ci]);
            res.cols.push_back(std::move(c));
        }
        return res;
    }

    std::string sql;
    if (backendIsPostgres(backend)) {
        sql =
            "SELECT i.relname AS INDEX_NAME, a.attname AS COLUMN_NAME, "
            "k.ord AS ORDINAL_POSITION, "
            "CASE WHEN ix.indisunique THEN 'NO' ELSE 'YES' END AS NON_UNIQUE, "
            "am.amname AS TYPE, 0 AS CARDINALITY, 0 AS PAGES "
            "FROM pg_catalog.pg_class t JOIN pg_catalog.pg_namespace n ON n.oid = t.relnamespace "
            "JOIN pg_catalog.pg_index ix ON t.oid = ix.indrelid "
            "JOIN pg_catalog.pg_class i ON i.oid = ix.indexrelid "
            "JOIN pg_catalog.pg_am am ON am.oid = i.relam "
            "LEFT JOIN LATERAL unnest(ix.indkey) WITH ORDINALITY AS k(attnum, ord) ON true "
            "LEFT JOIN pg_catalog.pg_attribute a ON a.attrelid = t.oid AND a.attnum = k.attnum "
            "WHERE n.nspname = current_schema() AND t.relname = " + sqlQuote(table) +
            " AND a.attname IS NOT NULL ORDER BY i.relname, k.ord";
    } else {
        sql =
            "SELECT INDEX_NAME AS INDEX_NAME, COLUMN_NAME AS COLUMN_NAME, "
            "SEQ_IN_INDEX AS ORDINAL_POSITION, "
            "CASE WHEN NON_UNIQUE = 0 THEN 'NO' ELSE 'YES' END AS NON_UNIQUE, "
            "INDEX_TYPE AS TYPE, CARDINALITY AS CARDINALITY, 0 AS PAGES "
            "FROM INFORMATION_SCHEMA.STATISTICS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = " + sqlQuote(table) +
            " ORDER BY INDEX_NAME, SEQ_IN_INDEX";
    }

    db::DBConnection *conn = db::getConnection(dsn, 0);
    DbInfoResult res;
    db::DBResult r = conn->execute(sql, -1);
    int nameIdx = findColIdx(r, "index_name");
    int colIdx = findColIdx(r, "column_name");
    int ordIdx = findColIdx(r, "ordinal_position");
    int nonUniqueIdx = findColIdx(r, "non_unique");
    int typeIdx = findColIdx(r, "type");
    int cardIdx = findColIdx(r, "cardinality");
    int pagesIdx = findColIdx(r, "pages");
    for (int ci = 0; ci < 7; ci++) {
        DbInfoCol c;
        c.name = kCols[ci];
        for (long long row = 0; row < r.rowCount; row++) {
            std::string cell;
            switch (ci) {
            case 0: cell = cellStr(r, nameIdx, row); break;
            case 1: cell = cellStr(r, colIdx, row); break;
            case 2: cell = cellStr(r, ordIdx, row); break;
            case 3: cell = cellStr(r, nonUniqueIdx, row); break;
            case 4: cell = cellStr(r, typeIdx, row); break;
            case 5: cell = cellStr(r, cardIdx, row); break;
            case 6: cell = cellStr(r, pagesIdx, row); break;
            }
            c.values.push_back(cfvariant(cell.c_str()));
        }
        res.cols.push_back(std::move(c));
    }
    res.rowCount = r.rowCount;
    return res;
}

} // namespace

void cf_dbinfo(const cfvariant *attrs,
               void *cgi, void *server, void *cookie, void *application,
               void *session, void *url, void *form, void *variables)
{
    auto attr = [&](const char *key) -> const cfvariant * {
        if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
        string k(key);
        auto it = attrs->m_struct->find(k);
        return it == attrs->m_struct->end() ? nullptr : &it->second;
    };

    std::string type = attr("type") ? safe_to_std_string(*attr("type")) : "tables";
    std::string dsn = attr("datasource") ? safe_to_std_string(*attr("datasource")) : "";
    std::string name = attr("name") ? safe_to_std_string(*attr("name")) : "";
    std::string pattern = attr("pattern") ? safe_to_std_string(*attr("pattern")) : "";
    std::string table = attr("table") ? safe_to_std_string(*attr("table")) : "";

    std::string typeUp;
    for (char c : type) typeUp += static_cast<char>(toupper((unsigned char)c));

    static const char *const kValidTypes[] = {
        "FOREIGNKEYS", "TABLES", "COLUMNS", "CLIENTINFO", "PROCEDURES", "INDEX", "VERSION", "DBNAMES"
    };
    bool valid = false;
    for (const char *t : kValidTypes) {
        if (typeUp == t) { valid = true; break; }
    }
    if (!valid) {
        throw webstrada::exception("Attribute validation error for CFDBINFO.",
            ("The value of the TYPE attribute, which is currently " + type + ", must be one of the values: FOREIGNKEYS,TABLES,COLUMNS,CLIENTINFO,PROCEDURES,INDEX,VERSION,DBNAMES.").c_str());
    }

    // name is required for the metadata types that produce a query.
    if (typeUp != "CLIENTINFO" && name.empty()) {
        throw webstrada::exception("Attribute validation error for tag CFDBINFO.",
            ("When the value of the TYPE attribute is " + typeUp + ", it requires the attribute(s): NAME.").c_str());
    }
    // table is required for columns / foreignkeys / index.
    if ((typeUp == "COLUMNS" || typeUp == "FOREIGNKEYS" || typeUp == "INDEX") && table.empty()) {
        throw webstrada::exception("Attribute validation error for tag CFDBINFO.",
            ("When the value of the TYPE attribute is " + typeUp + ", it requires the attribute(s): TABLE.").c_str());
    }

    std::string backend = dsnBackend(dsn);

    cfml::trace_record_event("DB_DBINFO_START", dsn.c_str(), typeUp.c_str(), 0);
    DbInfoResult res;
    if (typeUp == "TABLES") {
        res = dbinfoTables(backend, dsn, pattern);
    } else if (typeUp == "COLUMNS") {
        res = dbinfoColumns(backend, dsn, table);
    } else if (typeUp == "VERSION") {
        res = dbinfoVersion(backend, dsn);
    } else if (typeUp == "PROCEDURES") {
        res = dbinfoProcedures(backend, dsn);
    } else if (typeUp == "DBNAMES") {
        res = dbinfoDbNames(backend, dsn);
    } else if (typeUp == "FOREIGNKEYS") {
        res = dbinfoForeignKeys(backend, dsn, table);
    } else if (typeUp == "INDEX") {
        res = dbinfoIndex(backend, dsn, table);
    } else if (typeUp == "CLIENTINFO") {
        // CLIENTINFO is accepted but produces an empty result (no client
        // tracking in this engine).
        res.rowCount = 0;
    }
    cfml::trace_record_event("DB_DBINFO_END", dsn.c_str(), typeUp.c_str(), 0);

    storeResult(cgi, server, cookie, application, session, url, form, variables, name, res);
}

} // namespace cfml

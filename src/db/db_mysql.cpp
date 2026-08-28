/**
 * @file db_mysql.cpp
 * @brief MySQL/MariaDB implementation of the abstract database layer.
 *
 * Uses the MariaDB Connector/C client library (libmariadb, MySQL-compatible).
 * A datasource configured with backend="mysql" (webstrada::config::datasources)
 * connects to the server given by the host/port/database/username/password
 * fields. execute() runs every statement in the SQL script and surfaces only
 * the FIRST result, matching the abstract layer's contract (and CF's
 * Executive.getRowSet). Multiple statements are supported via the
 * CLIENT_MULTI_STATEMENTS capability. Transaction control uses BEGIN/COMMIT/
 * ROLLBACK and SAVEPOINT / ROLLBACK TO.
 */

#include <webstrada/db.h>
#include <webstrada/exceptions.h>
#include <webstrada/config.h>

#include <mysql.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace webstrada {
namespace db {

namespace {

// Map a MySQL field type to a CFML value (cell). `row` is a whole fetched row;
// `col` indexes into it; `lengths` (from mysql_fetch_lengths) carries the
// per-column byte lengths; `field` describes the column.
cfvariant mysqlCellToVariant(const MYSQL_ROW row, unsigned long *lengths,
                             unsigned int col, const MYSQL_FIELD *field)
{
    if (!row || !row[col]) return cfvariant(cfvariant::Null);
    enum_field_types ft = field ? field->type : MYSQL_TYPE_NULL;
    unsigned long len = lengths ? lengths[col] : 0;
    switch (ft) {
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONGLONG: {
        long long v = 0;
        if (row[col]) {
            const char *p = row[col];
            char *end = nullptr;
            v = strtoll(p, &end, 10);
        }
        if (v >= INT32_MIN && v <= INT32_MAX) return cfvariant(static_cast<int>(v));
        cfvariant c(cfvariant::Long);
        c.m_long = v;
        return c;
    }
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE: {
        double d = row[col] ? strtod(row[col], nullptr) : 0.0;
        cfvariant c(cfvariant::Float);
        c.m_double = d;
        return c;
    }
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL: {
        // CF stores DECIMAL as a java.math.BigDecimal: the value stays a
        // number (arithmetic coerces it) but it stringifies with its original
        // scale ("12.50", not "12.5" — verified on CF 2025). The engine has no
        // BigDecimal type, so preserve the raw server text as the Float's
        // literal text (the same mechanism float literals use).
        double d = row[col] ? strtod(row[col], nullptr) : 0.0;
        cfvariant c(cfvariant::Float);
        c.m_double = d;
        if (row[col]) c.m_literalText = new string(row[col]);
        return c;
    }
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB: {
        // A BLOB with a text charset (charsetnr != binary charset 63) is text
        // (TEXT/LONGTEXT columns, and metadata like SHOW FULL COLUMNS' Type
        // column report as text blobs). Only a true binary BLOB becomes a
        // Binary value.
        if (field && field->charsetnr != 63) {
            return row[col] ? cfvariant(row[col]) : cfvariant(cfvariant::Null);
        }
        cfvariant c(cfvariant::Binary);
        if (row[col] && lengths) {
            const unsigned char *b = reinterpret_cast<const unsigned char*>(row[col]);
            unsigned long n = lengths[col];
            c.m_binary->assign(reinterpret_cast<const std::byte*>(b),
                               reinterpret_cast<const std::byte*>(b) + n);
        }
        return c;
    }
    default:
        return row[col] ? cfvariant(row[col]) : cfvariant(cfvariant::Null);
    }
}

// The type hint CFML exposes for a column. Kept simple like the SQLite backend
// (which reports "varchar" for everything).
std::string mysqlTypeHint(enum_field_types ft)
{
    switch (ft) {
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONGLONG:
        return "integer";
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE:
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
        return "float";
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
        return "binary";
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_TIME:
        return "date";
    default:
        return "varchar";
    }
}

// CF-observable recordcount semantics for a first non-result statement,
// mirroring the SQLite backend's convention: INSERT/UPDATE/DELETE report their
// affected-row count; DDL (CREATE/DROP/ALTER/INDEX) reports 1. Returns
// (hasGeneratedKey, generatedKey).
void applyNonResultSemantics(MYSQL *conn, const std::string &sqlText,
                             long long &rowCount, long long &generatedKey,
                             bool &hasGeneratedKey)
{
    rowCount = static_cast<long long>(mysql_affected_rows(conn));
    std::string up = sqlText;
    for (auto &c : up) c = static_cast<char>(toupper(c));
    bool isDml = up.find("INSERT") != std::string::npos ||
                 up.find("UPDATE") != std::string::npos ||
                 up.find("DELETE") != std::string::npos ||
                 up.find("REPLACE") != std::string::npos;
    if (rowCount == 0 && !isDml) {
        rowCount = 1;
    }
    if (up.find("INSERT") != std::string::npos || up.find("REPLACE") != std::string::npos) {
        unsigned long long lid = mysql_insert_id(conn);
        if (lid != 0) {
            generatedKey = static_cast<long long>(lid);
            hasGeneratedKey = true;
        }
    }
}

// Build a DBResult (columns + rows) from a fetched MYSQL_RES. `maxrows` < 0
// reads all rows.
DBResult dbResultFromMysqlRes(MYSQL_RES *res, long long maxrows)
{
    DBResult result;
    unsigned int colCount = mysql_num_fields(res);
    MYSQL_FIELD *fields = mysql_fetch_fields(res);
    for (unsigned int c = 0; c < colCount; c++) {
        DBColumn col;
        col.name = fields[c].name ? fields[c].name : "";
        col.type = mysqlTypeHint(fields[c].type);
        result.columns.push_back(std::move(col));
    }
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        if (maxrows < 0 || result.rowCount < maxrows) {
            unsigned long *lengths = mysql_fetch_lengths(res);
            for (unsigned int c = 0; c < colCount; c++) {
                result.columns[c].values.push_back(mysqlCellToVariant(row, lengths, c, &fields[c]));
            }
            result.rowCount++;
        }
    }
    return result;
}

// Quote a string value as a MySQL string literal (backslash and single quote
// escaped).
std::string mysqlQuoteValue(const std::string &v)
{
    std::string out;
    out.reserve(v.size() + 2);
    out.push_back('\'');
    for (char c : v) {
        if (c == '\\' || c == '\'') out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

class MySqlConnection : public DBConnection
{
public:
    MySqlConnection(MYSQL *conn, const std::string &dsn) : m_conn(conn), m_dsn(dsn) {}
    ~MySqlConnection() override
    {
        if (m_conn) mysql_close(m_conn);
    }

    bool isAlive() override
    {
        return m_conn && mysql_ping(m_conn) == 0;
    }

    DBResult execute(const std::string &sql, long long maxrows) override
    {
        DBResult result;
        // MySQL's client library needs CLIENT_MULTI_STATEMENTS to run several
        // statements in one mysql_real_query call; it is set at connect time.
        if (mysql_real_query(m_conn, sql.c_str(), sql.size()) != 0) {
            throwDatabaseError(mysql_error(m_conn));
        }

        bool first = true;
        std::string firstSqlText = sql;
        do {
            MYSQL_RES *res = mysql_store_result(m_conn);
            if (res) {
                // A result set: fill columns/rows if it is the first one;
                // later result sets are consumed and discarded.
                if (first) {
                    result = dbResultFromMysqlRes(res, maxrows);
                }
                mysql_free_result(res);
                first = false;
            } else {
                // A non-result statement. The FIRST one reports its affected
                // rows / generated key; later ones are drained.
                if (mysql_errno(m_conn) != 0) {
                    throwDatabaseError(mysql_error(m_conn));
                }
                if (first) {
                    applyNonResultSemantics(m_conn, firstSqlText, result.rowCount,
                                            result.generatedKey, result.hasGeneratedKey);
                }
                first = false;
            }
        } while (mysql_next_result(m_conn) == 0);

        return result;
    }

    DBStoredProcResult storedProc(const std::string &proc,
                                  const std::vector<DBStoredProcParam> &params) override
    {
        DBStoredProcResult result;
        // Assign every in/inout parameter to a session variable
        // (@webstrada_sp_<i>), which both passes the value in and lets the out
        // values be read back with a SELECT. OUT-only params are not assigned.
        std::vector<std::string> outVars;
        for (size_t i = 0; i < params.size(); i++) {
            const auto &p = params[i];
            std::string var = "@webstrada_sp_" + std::to_string(i);
            if (p.type == "in" || p.type == "inout") {
                std::string lit = p.isNull ? "NULL" : mysqlQuoteValue(p.value);
                exec("SET " + var + " = " + lit);
            }
            if (p.type == "out" || p.type == "inout") outVars.push_back(var);
        }

        // CALL proc(@webstrada_sp_0, @webstrada_sp_1, ...); every result set the
        // procedure returns is collected.
        std::string callSql = "CALL " + proc + "(";
        for (size_t i = 0; i < params.size(); i++) {
            if (i) callSql += ", ";
            callSql += "@webstrada_sp_" + std::to_string(i);
        }
        callSql += ")";
        if (mysql_real_query(m_conn, callSql.c_str(), callSql.size()) != 0) {
            throwDatabaseError(mysql_error(m_conn));
        }
        do {
            MYSQL_RES *res = mysql_store_result(m_conn);
            if (res) {
                result.resultsets.push_back(dbResultFromMysqlRes(res, -1));
                mysql_free_result(res);
            } else if (mysql_errno(m_conn) != 0) {
                throwDatabaseError(mysql_error(m_conn));
            }
        } while (mysql_next_result(m_conn) == 0);

        // Read the out/inout values back.
        if (!outVars.empty()) {
            std::string sel = "SELECT ";
            for (size_t i = 0; i < outVars.size(); i++) {
                if (i) sel += ", ";
                sel += outVars[i];
            }
            if (mysql_real_query(m_conn, sel.c_str(), sel.size()) != 0) {
                throwDatabaseError(mysql_error(m_conn));
            }
            MYSQL_RES *res = mysql_store_result(m_conn);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                unsigned long *lengths = mysql_fetch_lengths(res);
                for (size_t i = 0; i < outVars.size(); i++) {
                    if (row && row[i]) {
                        result.outValues.push_back(std::string(row[i], lengths[i]));
                    } else {
                        result.outValues.push_back("");
                    }
                }
                mysql_free_result(res);
            }
        }

        // MySQL procedures have no return value, so the status code is 0.
        result.statusCode = 0;
        result.hasStatusCode = true;
        return result;
    }

    void begin() override { exec("BEGIN"); }
    void commit() override { exec("COMMIT"); }
    void rollback() override { exec("ROLLBACK"); }

    void setSavepoint(const std::string &name) override
    {
        exec("SAVEPOINT " + name);
    }

    void rollbackTo(const std::string &name) override
    {
        exec("ROLLBACK TO " + name);
    }

    std::vector<DBConnection::ColumnMeta> tableColumns(const std::string &table) override
    {
        std::vector<DBConnection::ColumnMeta> cols;
        // SHOW FULL COLUMNS returns: Field, Type, Collation, Null, Key,
        // Default, Extra, Privileges, Comment.
        DBResult r = execute("SHOW FULL COLUMNS FROM " + table, -1);
        int fieldIdx = -1, typeIdx = -1, keyIdx = -1;
        for (size_t i = 0; i < r.columns.size(); i++) {
            std::string cn;
            for (char c : r.columns[i].name) cn += static_cast<char>(tolower((unsigned char)c));
            if (cn == "field") fieldIdx = (int)i;
            else if (cn == "type") typeIdx = (int)i;
            else if (cn == "key") keyIdx = (int)i;
        }
        auto cellStr = [&](int colIdx, long long row) -> std::string {
            if (colIdx < 0 || (size_t)colIdx >= r.columns.size() ||
                (size_t)row >= r.columns[colIdx].values.size())
                return "";
            cfvariant &v = r.columns[colIdx].values[row];
            if (v.m_type == cfvariant::Null) return "";
            const char *p = v.toString().constData();
            return p ? std::string(p) : "";
        };
        for (long long row = 0; row < r.rowCount; row++) {
            DBConnection::ColumnMeta cm;
            cm.name = cellStr(fieldIdx, row);
            cm.type = cellStr(typeIdx, row);
            cm.isPk = (cellStr(keyIdx, row) == "PRI");
            cols.push_back(cm);
        }
        return cols;
    }

private:
    MYSQL *m_conn;
    std::string m_dsn;

    [[noreturn]] void throwDatabaseError(const std::string &detail) const
    {
        throw webstrada::exception("Database", "Error Executing Database Query.",
                                  detail.c_str());
    }

    void exec(const std::string &sql)
    {
        if (mysql_real_query(m_conn, sql.c_str(), sql.size()) != 0) {
            throwDatabaseError(mysql_error(m_conn));
        }
        // Drain any result set.
        MYSQL_RES *res = mysql_store_result(m_conn);
        if (res) mysql_free_result(res);
        while (mysql_next_result(m_conn) == 0) {
            MYSQL_RES *r2 = mysql_store_result(m_conn);
            if (r2) mysql_free_result(r2);
        }
    }
};

class MySqlDriver : public DBDriver
{
public:
    const char *name() const override { return "mysql"; }

    DBConnection *open(const std::string &dsn,
                       const std::string &host, int port,
                       const std::string &database,
                       const std::string &username,
                       const std::string &password,
                       long long timeoutMs) override
    {
        MYSQL *conn = mysql_init(nullptr);
        if (!conn) {
            throw webstrada::exception("Database", "Error Executing Database Query.",
                                      "mysql_init failed");
        }
        // Multi-statement support (matching CF's batch-capable drivers).
        unsigned long clientFlags = CLIENT_MULTI_STATEMENTS;

        const char *h = host.empty() ? "127.0.0.1" : host.c_str();
        const char *u = username.empty() ? "root" : username.c_str();
        const char *p = password.c_str();
        const char *db = database.empty() ? nullptr : database.c_str();
        unsigned int pn = port > 0 ? static_cast<unsigned int>(port) : 3306;

        MYSQL *ok = mysql_real_connect(conn, h, u, p, db, pn, nullptr, clientFlags);
        if (!ok) {
            std::string err = mysql_error(conn);
            mysql_close(conn);
            throw webstrada::exception("Database", "Error Executing Database Query.",
                ("Could not open the datasource '" + dsn + "' database (" + err + ").").c_str());
        }
        return new MySqlConnection(conn, dsn);
    }
};

MySqlDriver g_mySqlDriver;

} // namespace

// The MySQL driver is registered by db.cpp's registerBuiltinDrivers (which
// also registers the SQLite driver), so a single registration point pulls both
// backends into the link.
void registerMySqlBuiltinDriver()
{
    registerDriver(&g_mySqlDriver);
}

} // namespace db
} // namespace webstrada

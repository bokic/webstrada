/**
 * @file db_pgsql.cpp
 * @brief PostgreSQL implementation of the abstract database layer.
 *
 * Uses the PostgreSQL client library (libpq). A datasource configured with
 * backend="postgres" (or "postgresql" / "pg", see webstrada::config::datasources)
 * connects to the server given by the host/port/database/username/password
 * fields. execute() runs every statement in the SQL script (via the simple
 * query protocol, which executes a multi-statement string statement by
 * statement) and surfaces only the FIRST result, matching the abstract layer's
 * contract (and CF's Executive.getRowSet). Transaction control uses BEGIN/
 * COMMIT/ROLLBACK and SAVEPOINT / ROLLBACK TO SAVEPOINT.
 */

#include <webstrada/db.h>
#include <webstrada/exceptions.h>
#include <webstrada/config.h>

#include <libpq-fe.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace webstrada {
namespace db {

namespace {

// PostgreSQL catalog OIDs for the built-in types (from pg_type). Client code
// cannot include the server's pg_type_d.h, so the stable OIDs are spelled out.
constexpr Oid kBoolOid      = 16;
constexpr Oid kByteaOid     = 17;
constexpr Oid kInt8Oid      = 20;
constexpr Oid kInt2Oid      = 21;
constexpr Oid kInt4Oid      = 23;
constexpr Oid kOidOid       = 26;
constexpr Oid kFloat4Oid    = 700;
constexpr Oid kFloat8Oid    = 701;
constexpr Oid kDateOid      = 1082;
constexpr Oid kTimeOid      = 1083;
constexpr Oid kTimestampOid = 1114;
constexpr Oid kTimestamptzOid = 1184;
constexpr Oid kIntervalOid  = 1186;
constexpr Oid kTimetzOid    = 1266;
constexpr Oid kNumericOid   = 1700;
constexpr Oid kCharOid      = 1042;
constexpr Oid kVarcharOid   = 1043;
constexpr Oid kTextOid      = 25;

// Map a CF_SQL_* type name (normalized, no prefix) to a PostgreSQL OID, so a
// NULL parameter is passed with an explicit type (PG cannot infer a NULL's
// type from the value, which would make the CALL ambiguous). Returns 0 when the
// type is unknown, leaving the placeholder untyped (PG infers from context).
Oid pqOidForSqlType(const std::string &sqlType)
{
    if (sqlType == "INTEGER") return kInt4Oid;
    if (sqlType == "BIGINT") return kInt8Oid;
    if (sqlType == "SMALLINT" || sqlType == "TINYINT") return kInt2Oid;
    if (sqlType == "NUMERIC" || sqlType == "DECIMAL" || sqlType == "MONEY" ||
        sqlType == "MONEY4") return kNumericOid;
    if (sqlType == "FLOAT" || sqlType == "DOUBLE" || sqlType == "REAL") return kFloat8Oid;
    if (sqlType == "VARCHAR" || sqlType == "LONGVARCHAR") return kVarcharOid;
    if (sqlType == "CHAR") return kCharOid;
    if (sqlType == "DATE") return kDateOid;
    if (sqlType == "TIME") return kTimeOid;
    if (sqlType == "TIMESTAMP") return kTimestampOid;
    if (sqlType == "BIT") return kBoolOid;
    if (sqlType == "BINARY" || sqlType == "VARBINARY" || sqlType == "LONGVARBINARY")
        return kByteaOid;
    return 0;
}

int hexValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

// Decode a BYTEA column's text representation into raw bytes. The simple query
// protocol returns BYTEA either as `\x<hex>` (bytea_output = hex, the default
// since PostgreSQL 9.0) or as the legacy escape format (`\ooo` octal triplets,
// `\\` for a backslash).
std::vector<std::byte> decodeBytea(const char *s, int n)
{
    std::vector<std::byte> out;
    if (n >= 2 && s[0] == '\\' && s[1] == 'x') {
        out.reserve(static_cast<size_t>((n - 2) / 2));
        for (int i = 2; i + 1 < n; i += 2) {
            out.push_back(static_cast<std::byte>(
                static_cast<unsigned char>((hexValue(s[i]) << 4) | hexValue(s[i + 1]))));
        }
        return out;
    }
    for (int i = 0; i < n; i++) {
        if (s[i] == '\\' && i + 1 < n && s[i + 1] == '\\') {
            out.push_back(static_cast<std::byte>('\\'));
            i++;
        } else if (s[i] == '\\' && i + 1 < n && s[i + 1] >= '0' && s[i + 1] <= '7') {
            int v = 0, cnt = 0;
            while (i + 1 < n && cnt < 3 && s[i + 1] >= '0' && s[i + 1] <= '7') {
                v = v * 8 + (s[i + 1] - '0');
                i++;
                cnt++;
            }
            out.push_back(static_cast<std::byte>(static_cast<unsigned char>(v)));
        } else {
            out.push_back(static_cast<std::byte>(static_cast<unsigned char>(s[i])));
        }
    }
    return out;
}

// Map a PostgreSQL column type to a CFML value (cell). `res` holds the whole
// result; `row`/`col` select the cell.
cfvariant pgCellToVariant(PGresult *res, int row, int col)
{
    if (PQgetisnull(res, row, col)) return cfvariant(cfvariant::Null);
    Oid type = PQftype(res, col);
    const char *v = PQgetvalue(res, row, col);
    int len = PQgetlength(res, row, col);
    switch (type) {
    case kBoolOid: {
        // CF's PostgreSQL driver surfaces a bool column as a java.lang.Integer
        // 1/0 (verified on CF 2025: SELECT true -> "1", false -> "0",
        // q.b1.getClass() == java.lang.Integer, SerializeJSON -> 1). The simple
        // protocol returns 't'/'f'; treat '1'/'0' defensively too.
        bool b = v[0] == 't' || v[0] == 'T' || v[0] == '1';
        return cfvariant(b ? 1 : 0);
    }
    case kInt2Oid:
    case kInt4Oid:
    case kInt8Oid:
    case kOidOid: {
        long long n = strtoll(v, nullptr, 10);
        if (n >= INT32_MIN && n <= INT32_MAX) return cfvariant(static_cast<int>(n));
        cfvariant c(cfvariant::Long);
        c.m_long = n;
        return c;
    }
    case kFloat4Oid:
    case kFloat8Oid:
    case kNumericOid: {
        cfvariant c(cfvariant::Float);
        c.m_double = strtod(v, nullptr);
        return c;
    }
    case kByteaOid: {
        std::vector<std::byte> bytes = decodeBytea(v, len);
        cfvariant c(cfvariant::Binary);
        c.m_binary->assign(bytes.begin(), bytes.end());
        return c;
    }
    default:
        // Everything else (text/varchar/uuid/json/xml/date/time/...) keeps the
        // server's text representation, matching the SQLite/MySQL backends.
        return cfvariant(v);
    }
}

// The type hint CFML exposes for a column (mirrors the MySQL backend's hints;
// date/time columns keep their text representation like the MySQL backend).
std::string pgTypeHint(Oid type)
{
    switch (type) {
    case kInt2Oid:
    case kInt4Oid:
    case kInt8Oid:
    case kOidOid:
        return "integer";
    case kFloat4Oid:
    case kFloat8Oid:
    case kNumericOid:
        return "float";
    case kByteaOid:
        return "binary";
    case kDateOid:
    case kTimeOid:
    case kTimetzOid:
    case kTimestampOid:
    case kTimestamptzOid:
    case kIntervalOid:
        return "date";
    case kBoolOid:
        return "boolean";
    default:
        return "varchar";
    }
}

// CF-observable recordcount semantics for a first non-result statement,
// mirroring the SQLite/MySQL backends' convention: INSERT/UPDATE/DELETE report
// their affected-row count; DDL (CREATE/DROP/ALTER/INDEX) reports 1 because
// PQcmdTuples is empty for DDL. A generated key is only reported when the
// server assigns one: modern PostgreSQL tables have no implicit OIDs, so a
// plain INSERT yields no key (matching pgjdbc's getGeneratedKeys, which only
// returns rows when the statement uses RETURNING — and then the statement is a
// result set, not this path). Returns (hasGeneratedKey, generatedKey).
void applyNonResultSemantics(PGresult *res, const std::string &sqlText,
                             long long &rowCount, long long &generatedKey,
                             bool &hasGeneratedKey)
{
    const char *cmdTuples = PQcmdTuples(res);
    rowCount = (cmdTuples && cmdTuples[0]) ? strtoll(cmdTuples, nullptr, 10) : 0;
    std::string up = sqlText;
    for (auto &c : up) c = static_cast<char>(toupper((unsigned char)c));
    bool isDml = up.find("INSERT") != std::string::npos ||
                 up.find("UPDATE") != std::string::npos ||
                 up.find("DELETE") != std::string::npos ||
                 up.find("REPLACE") != std::string::npos;
    if (rowCount == 0 && !isDml) {
        rowCount = 1;
    }
    if (up.find("INSERT") != std::string::npos || up.find("REPLACE") != std::string::npos) {
        Oid oid = PQoidValue(res);
        if (oid != 0) {
            generatedKey = static_cast<long long>(oid);
            hasGeneratedKey = true;
        }
    }
}

// Build a DBResult (columns + rows) from a fetched PGresult.
DBResult dbResultFromPgResult(PGresult *res)
{
    DBResult result;
    int colCount = PQnfields(res);
    for (int c = 0; c < colCount; c++) {
        DBColumn col;
        const char *name = PQfname(res, c);
        col.name = name ? name : "";
        col.type = pgTypeHint(PQftype(res, c));
        result.columns.push_back(std::move(col));
    }
    int rowCount = PQntuples(res);
    for (int r = 0; r < rowCount; r++) {
        for (int c = 0; c < colCount; c++) {
            result.columns[c].values.push_back(pgCellToVariant(res, r, c));
        }
        result.rowCount++;
    }
    return result;
}

class PostgresConnection : public DBConnection
{
public:
    PostgresConnection(PGconn *conn, const std::string &dsn)
        : m_conn(conn), m_dsn(dsn) {}
    ~PostgresConnection() override
    {
        if (m_conn) PQfinish(m_conn);
    }

    DBResult execute(const std::string &sql, long long maxrows) override
    {
        DBResult result;
        // The simple query protocol executes every statement in the script and
        // returns one result per statement via PQgetResult; the FIRST result is
        // surfaced, later ones are consumed and discarded (matching CF's
        // Executive.getRowSet and the MySQL backend's CLIENT_MULTI_STATEMENTS
        // loop).
        if (PQsendQuery(m_conn, sql.c_str()) != 1) {
            throwDatabaseError(PQerrorMessage(m_conn));
        }
        bool first = true;
        std::string firstSqlText = sql;
        while (PGresult *res = PQgetResult(m_conn)) {
            ExecStatusType st = PQresultStatus(res);
            if (st == PGRES_TUPLES_OK) {
                if (first) {
                    result = dbResultFromPgResult(res);
                    if (maxrows >= 0 && result.rowCount > maxrows) {
                        for (auto &col : result.columns)
                            col.values.resize(static_cast<size_t>(maxrows));
                        result.rowCount = maxrows;
                    }
                }
                first = false;
            } else if (st == PGRES_COMMAND_OK) {
                if (first) {
                    applyNonResultSemantics(res, firstSqlText, result.rowCount,
                                            result.generatedKey, result.hasGeneratedKey);
                }
                first = false;
            } else if (st == PGRES_EMPTY_QUERY) {
                // Trailing empty statement (e.g. an extra semicolon): not a
                // result, leave `first` untouched.
            } else {
                std::string err = PQresultErrorMessage(res);
                PQclear(res);
                throwDatabaseError(err);
            }
            PQclear(res);
        }
        if (PQstatus(m_conn) != CONNECTION_OK) {
            throwDatabaseError(PQerrorMessage(m_conn));
        }
        return result;
    }

    DBStoredProcResult storedProc(const std::string &proc,
                                  const std::vector<DBStoredProcParam> &params) override
    {
        DBStoredProcResult result;
        // CALL proc($1, ..., $n) via the extended query protocol so a
        // procedure's OUT/INOUT values come back as a one-row result. OUT-only
        // params are passed as NULL (the value is ignored). An INOUT param's
        // declared value goes in.
        std::string sql = "CALL " + proc + "(";
        std::vector<const char*> values;
        std::vector<int> lengths;
        std::vector<int> formats;
        std::vector<Oid> types;
        bool hasOut = false;
        for (size_t i = 0; i < params.size(); i++) {
            if (i) sql += ", ";
            sql += "$" + std::to_string(i + 1);
            const auto &p = params[i];
            bool out = (p.type == "out" || p.type == "inout");
            if (out) hasOut = true;
            // An INOUT param still passes its input value; only an OUT-only
            // param's placeholder is NULL (its value is ignored).
            bool outOnly = (p.type == "out");
            if (p.isNull || outOnly) {
                values.push_back(nullptr);
                lengths.push_back(0);
            } else {
                values.push_back(p.value.c_str());
                lengths.push_back(static_cast<int>(p.value.size()));
            }
            formats.push_back(0); // text format
            // Type the placeholder from the CF_SQL_* type when known, so a NULL
            // parameter does not make the CALL ambiguous ("sp_add(unknown, ...)").
            types.push_back(pqOidForSqlType(p.cfsqltype));
        }
        sql += ")";

        if (PQsendQueryParams(m_conn, sql.c_str(), static_cast<int>(params.size()),
                              types.data(), values.data(), lengths.data(),
                              formats.data(), 0) != 1) {
            throwDatabaseError(PQerrorMessage(m_conn));
        }

        bool first = true;
        while (PGresult *res = PQgetResult(m_conn)) {
            ExecStatusType st = PQresultStatus(res);
            if (st == PGRES_TUPLES_OK) {
                if (first && hasOut) {
                    // The first result of CALL carries the OUT/INOUT values:
                    // one row, one column per out param in declaration order.
                    int cols = PQnfields(res);
                    for (int c = 0; c < cols; c++) {
                        if (PQntuples(res) > 0 && !PQgetisnull(res, 0, c)) {
                            result.outValues.push_back(PQgetvalue(res, 0, c));
                        } else {
                            result.outValues.push_back("");
                        }
                    }
                } else {
                    result.resultsets.push_back(dbResultFromPgResult(res));
                }
                first = false;
            } else if (st == PGRES_COMMAND_OK) {
                first = false;
            } else if (st == PGRES_EMPTY_QUERY) {
                // skip
            } else {
                std::string err = PQresultErrorMessage(res);
                PQclear(res);
                throwDatabaseError(err);
            }
            PQclear(res);
        }

        // PostgreSQL procedures have no return value, so the status code is 0.
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
        exec("ROLLBACK TO SAVEPOINT " + name);
    }

    std::vector<DBConnection::ColumnMeta> tableColumns(const std::string &table) override
    {
        std::vector<DBConnection::ColumnMeta> cols;
        // INFORMATION_SCHEMA.COLUMNS (in the connection's current schema) joined
        // with the PRIMARY KEY usage so <cfinsert>/<cfupdate>/<cfdbinfo> get the
        // name, declared data type and primary-key flag.
        std::string esc;
        esc.reserve(table.size());
        for (char c : table) {
            if (c == '\'') esc += "''";
            else esc += c;
        }
        std::string sql =
            "SELECT c.column_name, c.data_type, "
            "CASE WHEN kcu.column_name IS NOT NULL THEN true ELSE false END AS is_pk "
            "FROM information_schema.columns c "
            "LEFT JOIN information_schema.table_constraints tc "
            "  ON tc.table_name = c.table_name AND tc.table_schema = c.table_schema "
            " AND tc.constraint_type = 'PRIMARY KEY' "
            "LEFT JOIN information_schema.key_column_usage kcu "
            "  ON kcu.constraint_name = tc.constraint_name "
            " AND kcu.table_schema = tc.table_schema AND kcu.column_name = c.column_name "
            "WHERE c.table_name = '" + esc + "' AND c.table_schema = current_schema() "
            "ORDER BY c.ordinal_position";
        DBResult r = execute(sql, -1);
        int nameIdx = -1, typeIdx = -1, pkIdx = -1;
        for (size_t i = 0; i < r.columns.size(); i++) {
            std::string cn;
            for (char c : r.columns[i].name) cn += static_cast<char>(tolower((unsigned char)c));
            if (cn == "column_name") nameIdx = (int)i;
            else if (cn == "data_type") typeIdx = (int)i;
            else if (cn == "is_pk") pkIdx = (int)i;
        }
        for (long long row = 0; row < r.rowCount; row++) {
            DBConnection::ColumnMeta cm;
            if (nameIdx >= 0 && (size_t)nameIdx < r.columns.size() &&
                (size_t)row < r.columns[nameIdx].values.size())
                cm.name = std::string(r.columns[nameIdx].values[row].toString().constData());
            if (typeIdx >= 0 && (size_t)typeIdx < r.columns.size() &&
                (size_t)row < r.columns[typeIdx].values.size())
                cm.type = std::string(r.columns[typeIdx].values[row].toString().constData());
            if (pkIdx >= 0 && (size_t)pkIdx < r.columns.size() &&
                (size_t)row < r.columns[pkIdx].values.size())
                cm.isPk = (r.columns[pkIdx].values[row].m_type == cfvariant::Number &&
                           r.columns[pkIdx].values[row].m_int != 0);
            cols.push_back(cm);
        }
        return cols;
    }

private:
    PGconn *m_conn;
    std::string m_dsn;

    [[noreturn]] void throwDatabaseError(const std::string &detail) const
    {
        throw webstrada::exception("Database", "Error Executing Database Query.",
                                  detail.c_str());
    }

    void exec(const std::string &sql)
    {
        PGresult *res = PQexec(m_conn, sql.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string err = PQresultErrorMessage(res);
            PQclear(res);
            throwDatabaseError(err);
        }
        PQclear(res);
    }
};

// Escape a value for use inside single quotes in a libpq conninfo string
// (backslash escapes the following character there).
std::string conninfoQuote(const std::string &v)
{
    std::string out;
    out.reserve(v.size() + 2);
    for (char c : v) {
        if (c == '\\' || c == '\'') out += '\\';
        out += c;
    }
    return out;
}

class PostgresDriver : public DBDriver
{
public:
    explicit PostgresDriver(const char *name) : m_name(name) {}

    const char *name() const override { return m_name; }

    DBConnection *open(const std::string &dsn,
                       const std::string &host, int port,
                       const std::string &database,
                       const std::string &username,
                       const std::string &password,
                       long long timeoutMs) override
    {
        std::string conninfo = "host='" + conninfoQuote(host.empty() ? "127.0.0.1" : host) + "'";
        if (port > 0) conninfo += " port=" + std::to_string(port);
        if (!database.empty()) conninfo += " dbname='" + conninfoQuote(database) + "'";
        if (!username.empty()) conninfo += " user='" + conninfoQuote(username) + "'";
        if (!password.empty()) conninfo += " password='" + conninfoQuote(password) + "'";
        if (timeoutMs > 0) {
            conninfo += " connect_timeout=" + std::to_string(timeoutMs / 1000);
        }

        PGconn *conn = PQconnectdb(conninfo.c_str());
        if (PQstatus(conn) != CONNECTION_OK) {
            std::string err = PQerrorMessage(conn);
            PQfinish(conn);
            throw webstrada::exception("Database", "Error Executing Database Query.",
                ("Could not open the datasource '" + dsn + "' database (" + err + ").").c_str());
        }
        return new PostgresConnection(conn, dsn);
    }

private:
    const char *m_name;
};

PostgresDriver g_pgDriver("postgres");
PostgresDriver g_pgDriverAlias("postgresql");
PostgresDriver g_pgDriverAlias2("pg");

} // namespace

// The PostgreSQL driver is registered by db.cpp's registerBuiltinDrivers
// (which also registers the SQLite and MySQL drivers). The aliases ("postgres",
// "postgresql", "pg") all open PostgreSQL connections so any of them works in a
// datasource's backend= setting.
void registerPostgresBuiltinDriver()
{
    registerDriver(&g_pgDriver);
    registerDriver(&g_pgDriverAlias);
    registerDriver(&g_pgDriverAlias2);
}

} // namespace db
} // namespace webstrada

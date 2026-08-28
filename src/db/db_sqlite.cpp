/**
 * @file db_sqlite.cpp
 * @brief SQLite implementation of the abstract database layer.
 *
 * Every datasource is one SQLite file (`DNS_<name>.sqlite` next to the
 * executable, or in `webstrada::config::dsnDbDir` when set). The driver opens a
 * connection in WAL mode with a busy timeout; execute() runs every statement
 * in the SQL script (matching CF's batch-capable drivers) and surfaces only the
 * FIRST result, reproducing the recordcount / generated-key semantics that were
 * verified against CF's own SQLite driver.
 */

#include <webstrada/db.h>
#include <webstrada/exceptions.h>
#include <webstrada/config.h>

#include <sqlite3.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>
#include <unistd.h>

namespace webstrada {
namespace db {

namespace {

std::string dsnDbDirectory()
{
    if (!config::dsnDbDir.empty()) return config::dsnDbDir;
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n > 0) {
        exe[n] = '\0';
        std::string path(exe);
        size_t slash = path.find_last_of('/');
        return (slash != std::string::npos) ? path.substr(0, slash + 1) : std::string();
    }
    return std::string();
}

cfvariant sqliteCellToVariant(sqlite3_stmt *stmt, int col)
{
    switch (sqlite3_column_type(stmt, col)) {
    case SQLITE_INTEGER: {
        sqlite3_int64 v = sqlite3_column_int64(stmt, col);
        if (v >= INT32_MIN && v <= INT32_MAX) return cfvariant(static_cast<int>(v));
        cfvariant c(cfvariant::Long);
        c.m_long = static_cast<long long>(v);
        return c;
    }
    case SQLITE_FLOAT: {
        cfvariant c(cfvariant::Float);
        c.m_double = sqlite3_column_double(stmt, col);
        return c;
    }
    case SQLITE_TEXT: {
        const unsigned char *t = sqlite3_column_text(stmt, col);
        return t ? cfvariant(reinterpret_cast<const char*>(t)) : cfvariant(cfvariant::Null);
    }
    case SQLITE_BLOB: {
        cfvariant c(cfvariant::Binary);
        const void *b = sqlite3_column_blob(stmt, col);
        int n = sqlite3_column_bytes(stmt, col);
        c.m_binary->assign(static_cast<const std::byte*>(b),
                           static_cast<const std::byte*>(b) + n);
        return c;
    }
    default:
        return cfvariant(cfvariant::Null);
    }
}

// CF-observable recordcount semantics for a first non-result statement,
// verified against CF 2025's SQLite driver: INSERT/UPDATE/DELETE report their
// affected-row count; DDL (CREATE/DROP/ALTER/INDEX) reports 1 because
// sqlite3_changes() is 0 for DDL. Returns (hasGeneratedKey, generatedKey).
void applyNonResultSemantics(sqlite3 *db, sqlite3_stmt *stmt,
                             long long &rowCount, long long &generatedKey,
                             bool &hasGeneratedKey)
{
    rowCount = sqlite3_changes(db);
    std::string up = sqlite3_sql(stmt) ? sqlite3_sql(stmt) : "";
    for (auto &c : up) c = static_cast<char>(toupper(c));
    bool isDml = up.find("INSERT") != std::string::npos ||
                 up.find("UPDATE") != std::string::npos ||
                 up.find("DELETE") != std::string::npos ||
                 up.find("REPLACE") != std::string::npos;
    if (rowCount == 0 && !isDml) {
        rowCount = 1;
    }
    if (up.find("INSERT") != std::string::npos) {
        sqlite3_int64 lid = sqlite3_last_insert_rowid(db);
        if (lid != 0) {
            generatedKey = static_cast<long long>(lid);
            hasGeneratedKey = true;
        }
    }
}

class SqliteConnection : public DBConnection
{
public:
    explicit SqliteConnection(sqlite3 *db) : m_db(db) {}
    ~SqliteConnection() override
    {
        if (m_db) sqlite3_close(m_db);
    }

    bool isAlive() override
    {
        return m_db != nullptr;
    }

    DBResult execute(const std::string &sql, long long maxrows) override
    {
        DBResult result;
        std::string sqlErr;
        const char *pzTail = sql.c_str();
        bool first = true;
        sqlite3_stmt *stmt = nullptr;
        while (true) {
            int rc = sqlite3_prepare_v2(m_db, pzTail, -1, &stmt, &pzTail);
            if (rc != SQLITE_OK) {
                sqlErr = m_db ? sqlite3_errmsg(m_db) : "sqlite3_prepare failed";
                throwDatabaseError(sqlErr);
            }
            if (!stmt) break; // only whitespace / semicolons / comments remain

            int colCount = sqlite3_column_count(stmt);
            if (first && colCount > 0) {
                for (int c = 0; c < colCount; c++) {
                    DBColumn col;
                    const char *name = sqlite3_column_name(stmt, c);
                    col.name = name ? name : "";
                    col.type = "varchar";
                    result.columns.push_back(std::move(col));
                }
                rc = sqlite3_step(stmt);
                while (rc == SQLITE_ROW) {
                    if (maxrows < 0 || result.rowCount < maxrows) {
                        for (int c = 0; c < colCount; c++) {
                            result.columns[c].values.push_back(sqliteCellToVariant(stmt, c));
                        }
                        result.rowCount++;
                    }
                    rc = sqlite3_step(stmt);
                }
            } else {
                // The first non-result statement (report its update count /
                // auto key) or a trailing statement (run to completion, results
                // dropped).
                rc = sqlite3_step(stmt);
                while (rc == SQLITE_ROW) rc = sqlite3_step(stmt);
                if (first) {
                    applyNonResultSemantics(m_db, stmt, result.rowCount,
                                            result.generatedKey, result.hasGeneratedKey);
                }
            }
            sqlite3_finalize(stmt);
            first = false;
            if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
                sqlErr = m_db ? sqlite3_errmsg(m_db) : "sqlite3_step failed";
                throwDatabaseError(sqlErr);
            }
        }
        return result;
    }

    void begin() override { exec("BEGIN;"); }
    void commit() override { exec("COMMIT;"); }
    void rollback() override { exec("ROLLBACK;"); }

    void setSavepoint(const std::string &name) override
    {
        exec("SAVEPOINT " + name + ";");
    }

    void rollbackTo(const std::string &name) override
    {
        exec("ROLLBACK TO " + name + ";");
    }

    std::vector<DBConnection::ColumnMeta> tableColumns(const std::string &table) override
    {
        std::vector<DBConnection::ColumnMeta> cols;
        // PRAGMA table_info returns rows: cid, name, type, notnull, dflt_value, pk
        std::string sql = "PRAGMA table_info(";
        sql += table;
        sql += ")";
        DBResult r = execute(sql, -1);
        int nameIdx = -1, typeIdx = -1, pkIdx = -1;
        for (size_t i = 0; i < r.columns.size(); i++) {
            std::string cn;
            for (char c : r.columns[i].name) cn += static_cast<char>(tolower((unsigned char)c));
            if (cn == "name") nameIdx = (int)i;
            else if (cn == "type") typeIdx = (int)i;
            else if (cn == "pk") pkIdx = (int)i;
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
                           r.columns[pkIdx].values[row].m_int > 0);
            cols.push_back(cm);
        }
        return cols;
    }

private:
    sqlite3 *m_db;

    [[noreturn]] void throwDatabaseError(const std::string &detail) const
    {
        throw webstrada::exception("Database", "Error Executing Database Query.",
                                  detail.c_str());
    }

    void exec(const std::string &sql)
    {
        int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::string err = m_db ? sqlite3_errmsg(m_db) : "sqlite3_exec failed";
            throwDatabaseError(err);
        }
    }
};

class SqliteDriver : public DBDriver
{
public:
    const char *name() const override { return "sqlite"; }

    DBConnection *open(const std::string &dsn,
                       const std::string &host, int port,
                       const std::string &database,
                       const std::string &username,
                       const std::string &password,
                       long long timeoutMs) override
    {
        (void)host; (void)port; (void)database; (void)username; (void)password;

        std::string dbPath = dsnDbDirectory();
        if (!dbPath.empty() && dbPath.back() != '/') dbPath += "/";
        dbPath += "DNS_" + dsn + ".sqlite";

        sqlite3 *db = nullptr;
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::string err = db ? sqlite3_errmsg(db) : "sqlite3_open failed";
            if (db) sqlite3_close(db);
            throw webstrada::exception("Database", "Error Executing Database Query.",
                ("Could not open the datasource '" + dsn + "' database (" + err + ").").c_str());
        }
        sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        sqlite3_busy_timeout(db, timeoutMs > 0 ? static_cast<int>(timeoutMs) : 5000);
        return new SqliteConnection(db);
    }
};

SqliteDriver g_sqliteDriver;

} // namespace

// The SQLite driver is registered by db.cpp's registerBuiltinDrivers (which
// also registers the MySQL driver), so a single registration point pulls both
// backends into the link.
void registerSqliteBuiltinDriver()
{
    registerDriver(&g_sqliteDriver);
}

} // namespace db
} // namespace webstrada

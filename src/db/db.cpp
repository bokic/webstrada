/**
 * @file db.cpp
 * @brief Abstract database layer: driver registry and connection factory.
 */

#include <webstrada/db.h>
#include <webstrada/exceptions.h>
#include <webstrada/config.h>

#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace webstrada {
namespace db {

namespace {

std::map<std::string, DBDriver*> &drivers()
{
    static std::map<std::string, DBDriver*> reg;
    return reg;
}

// Dumps every operation on the abstract DB layer to stdout when
// webstrada::config::enableQueryLogging is on (the same flag that gates the
// [cfquery] / [cfstoredproc] lines in the CFML layer, see tag_query.cpp). The
// CLI and unit-test binary disable it because their stdout is a data channel
// (verify_with_coldfusion.py compares it byte-for-byte against CF). The wrapper
// owns the inner connection and forwards every operation, logging the SQL /
// arguments BEFORE delegating so even a statement that then fails is visible.
class LoggingConnection : public DBConnection
{
public:
    LoggingConnection(DBConnection *inner, const std::string &dsn,
                      const std::string &backend)
        : m_inner(inner), m_dsn(dsn), m_backend(backend) {}
    LoggingConnection(const LoggingConnection &) = delete;
    LoggingConnection &operator=(const LoggingConnection &) = delete;
    ~LoggingConnection() override
    {
        logOp("close");
        delete m_inner;
    }

    DBResult execute(const std::string &sql, long long maxrows) override
    {
        if (config::enableQueryLogging) {
            printf("[db] %s execute dsn=%s maxrows=%lld\n%s\n",
                   m_backend.c_str(), m_dsn.c_str(),
                   static_cast<long long>(maxrows), sql.c_str());
            fflush(stdout);
        }
        return m_inner->execute(sql, maxrows);
    }

    void begin() override { logOp("begin"); m_inner->begin(); }
    void commit() override { logOp("commit"); m_inner->commit(); }
    void rollback() override { logOp("rollback"); m_inner->rollback(); }

    void setSavepoint(const std::string &name) override
    {
        logOp("savepoint " + name);
        m_inner->setSavepoint(name);
    }

    void rollbackTo(const std::string &name) override
    {
        logOp("rollbackTo " + name);
        m_inner->rollbackTo(name);
    }

    std::vector<DBConnection::ColumnMeta> tableColumns(const std::string &table) override
    {
        logOp("tableColumns " + table);
        return m_inner->tableColumns(table);
    }

    DBStoredProcResult storedProc(const std::string &proc,
                                  const std::vector<DBStoredProcParam> &params) override
    {
        if (config::enableQueryLogging) {
            printf("[db] %s storedProc dsn=%s proc=%s params=%zu\n",
                   m_backend.c_str(), m_dsn.c_str(), proc.c_str(), params.size());
            fflush(stdout);
        }
        return m_inner->storedProc(proc, params);
    }

private:
    DBConnection *m_inner;
    std::string m_dsn;
    std::string m_backend;

    void logOp(const std::string &op)
    {
        if (config::enableQueryLogging) {
            printf("[db] %s %s dsn=%s\n", m_backend.c_str(), op.c_str(), m_dsn.c_str());
            fflush(stdout);
        }
    }
};

} // namespace

void registerDriver(DBDriver *driver)
{
    if (!driver) return;
    drivers()[driver->name()] = driver;
}

// Default stored-procedure support: backends without stored procedures
// (SQLite) throw; MySQL/PostgreSQL override storedProc() with CALL.
DBStoredProcResult DBConnection::storedProc(const std::string &proc,
                                            const std::vector<DBStoredProcParam> &params)
{
    throw webstrada::exception("Database", "Error Executing Database Query.",
        ("Stored procedures are not supported by this database backend (procedure '" + proc + "').").c_str());
}

// Backend registration helpers (db_sqlite.cpp / db_mysql.cpp / db_pgsql.cpp);
// registered by registerBuiltinDrivers.
void registerSqliteBuiltinDriver();
#ifdef WEBSTRADA_HAVE_MYSQL
void registerMySqlBuiltinDriver();
#endif
#ifdef WEBSTRADA_HAVE_POSTGRES
void registerPostgresBuiltinDriver();
#endif

void registerBuiltinDrivers()
{
    registerSqliteBuiltinDriver();
#ifdef WEBSTRADA_HAVE_MYSQL
    registerMySqlBuiltinDriver();
#endif
#ifdef WEBSTRADA_HAVE_POSTGRES
    registerPostgresBuiltinDriver();
#endif
}

// Resolved connection parameters for a datasource: the selected backend
// ("sqlite", "mysql", "postgres"; the default when the config has none) plus
// the server connection fields. Datasource names are case-insensitive (like
// CF), so the config key is matched with the lookup folded to upper case.
DatasourceConfig datasourceConfig(const std::string &dsn)
{
    DatasourceConfig cfg;
    cfg.backend = "sqlite";
    {
        std::string dsnUp = dsn;
        for (auto &c : dsnUp) c = static_cast<char>(toupper((unsigned char)c));
        auto it = config::datasources.find(dsnUp);
        if (it == config::datasources.end()) it = config::datasources.find(dsn);
        if (it != config::datasources.end()) {
            if (!it->second.backend.empty()) cfg.backend = it->second.backend;
            cfg.host = it->second.host;
            cfg.port = it->second.port;
            cfg.database = it->second.database;
            cfg.username = it->second.username;
            cfg.password = it->second.password;
        }
    }
    return cfg;
}

DBConnection *openConnection(const std::string &dsn, long long timeoutMs)
{
    // Lazily register the built-in backends so the layer always has defaults.
    if (drivers().empty()) registerBuiltinDrivers();

    // Select the backend from the datasource config (see datasourceConfig):
    // backend="mysql" uses the MySQL driver, backend="postgres"/"postgresql"/
    // "pg" the PostgreSQL driver; anything else (or no config entry) is SQLite.
    DatasourceConfig cfg = datasourceConfig(dsn);

    auto drv = drivers().find(cfg.backend);
    if (drv == drivers().end()) {
        throw webstrada::exception("Database", "Error Executing Database Query.",
            ("No database driver registered for backend '" + cfg.backend + "'.").c_str());
    }
    DBConnection *conn = drv->second->open(dsn, cfg.host, cfg.port, cfg.database, cfg.username,
                                           cfg.password, timeoutMs);
    if (config::enableQueryLogging) {
        printf("[db] %s open dsn=%s\n", cfg.backend.c_str(), dsn.c_str());
        fflush(stdout);
    }
    return new LoggingConnection(conn, dsn, cfg.backend);
}

} // namespace db
} // namespace webstrada

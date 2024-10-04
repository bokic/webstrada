/**
 * @file db.cpp
 * @brief Abstract database layer: driver registry and connection factory.
 */

#include <webstrada/db.h>
#include <webstrada/exceptions.h>
#include <webstrada/config.h>

#include <cctype>
#include <map>
#include <string>

namespace webstrada {
namespace db {

namespace {

std::map<std::string, DBDriver*> &drivers()
{
    static std::map<std::string, DBDriver*> reg;
    return reg;
}

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
    return drv->second->open(dsn, cfg.host, cfg.port, cfg.database, cfg.username,
                             cfg.password, timeoutMs);
}

} // namespace db
} // namespace webstrada

#pragma once

#include "cfvariant.h"

#include <string>
#include <vector>

// Abstract database layer. The CFML runtime (<cfquery> / queryExecute() /
// <cftransaction>) talks only to these interfaces; concrete backends (SQLite,
// MySQL, PostgreSQL) implement them. A datasource is named by a DSN string; the
// driver registry maps the backend for a connection. This lets future backends
// (SQL Server, Oracle, ...) be added without touching the CFML layer.

namespace webstrada {
namespace db {

// A single column of a query result: the column name (original case), a CFML
// type hint, and the column-major cell values (one cfvariant per row).
struct DBColumn
{
    std::string name;
    std::string type;                  // "varchar", "integer", ...
    std::vector<cfvariant> values;
};

// Result of executing a SQL script. `columns` is non-empty only when the FIRST
// statement produced a result set (later result sets are consumed and
// discarded, matching CF's Executive.getRowSet). For a first non-result
// statement, `rowCount` carries the affected-row count (or 1 for DDL, matching
// CF's SQLite driver) and `generatedKey` the auto-increment key of a single-row
// INSERT.
struct DBResult
{
    std::vector<DBColumn> columns;
    long long rowCount = 0;
    long long generatedKey = 0;
    bool hasGeneratedKey = false;
};

// A stored-procedure parameter (cfprocparam). `type` is "in"/"out"/"inout"
// (lowercase); `name` carries the dbVarName when the call uses named
// parameters; `cfsqltype` is the CF_SQL_* name (uppercased); `value` is the
// input value's string form; `isNull` forces a SQL NULL.
struct DBStoredProcParam
{
    std::string type = "in";
    std::string name;       // dbVarName ("" for positional)
    std::string cfsqltype;  // e.g. "CF_SQL_INTEGER"
    std::string value;
    bool isNull = false;
};

// Result of a stored-procedure call: every result set the procedure returned
// (in server order), the status code (when the backend reports one), and the
// out/inout parameter values in declaration order.
struct DBStoredProcResult
{
    std::vector<DBResult> resultsets;
    long long statusCode = 0;
    bool hasStatusCode = false;
    std::vector<std::string> outValues; // one per out/inout param, declaration order
};

// Abstract database connection. Implementations are not thread-safe: the
// engine opens one per request/transaction as needed.
class DBConnection
{
public:
    virtual ~DBConnection() = default;

    // Executes the whole SQL script (which may contain multiple statements,
    // executed in order like a JDBC Statement). Only the FIRST result is
    // surfaced; `maxrows` < 0 reads all rows. Throws
    // webstrada::exception("Database", "Error Executing Database Query.",
    // detail) on failure.
    virtual DBResult execute(const std::string &sql, long long maxrows) = 0;

    // Transaction control used by <cftransaction> / the transaction*()
    // built-ins.
    virtual void begin() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual void setSavepoint(const std::string &name) = 0;
    virtual void rollbackTo(const std::string &name) = 0;

    // Returns the named table's columns (name, declared type, and whether the
    // column is part of the primary key), used by <cfinsert>/<cfupdate>/
    // <cfdbinfo>. Backends implement this with their schema-introspection
    // syntax (SQLite: PRAGMA table_info; MySQL: SHOW COLUMNS / INFORMATION_SCHEMA).
    struct ColumnMeta {
        std::string name;
        std::string type;
        bool isPk = false;
    };
    virtual std::vector<ColumnMeta> tableColumns(const std::string &table) = 0;

    // Executes a stored procedure (`proc`) with the given parameters and
    // returns its result sets and out/inout values (used by <cfstoredproc>).
    // The default implementation throws: backends without stored procedures
    // (SQLite) cannot support this. MySQL/PG override it with CALL.
    virtual DBStoredProcResult storedProc(const std::string &proc,
                                          const std::vector<DBStoredProcParam> &params);
};

// A driver opens connections for a datasource. Connection parameters (host /
// port / database / username / password) come from the datasource config; a
// driver that does not use them (SQLite) ignores them.
class DBDriver
{
public:
    virtual ~DBDriver() = default;
    virtual const char *name() const = 0;   // e.g. "sqlite", "mysql"
    // Opens a connection to `dsn`. `params` carries the datasource config's
    // connection parameters (host/port/database/username/password). `timeoutMs`
    // <= 0 selects the driver default. Throws webstrada::exception("Database",
    // ...) when the datasource cannot be opened.
    virtual DBConnection *open(const std::string &dsn,
                               const std::string &host, int port,
                               const std::string &database,
                               const std::string &username,
                               const std::string &password,
                               long long timeoutMs) = 0;
};

// Driver registry. Drivers call registerDriver; openConnection lazily
// registers the built-in drivers (SQLite + MySQL + PostgreSQL) on first use, so
// the DB layer has working defaults even before any datasource configuration
// exists.
void registerDriver(DBDriver *driver);

// Resolved connection parameters for a datasource, shared by openConnection
// (which opens the connection) and the metadata layer (which dispatches
// backend-specific introspection, e.g. <cfdbinfo>). `backend` is "sqlite",
// "mysql" or "postgres" (the default "sqlite" when the datasource config has no
// backend, or no config entry exists); the server fields carry the config's
// host/port/database/username/password.
struct DatasourceConfig {
    std::string backend = "sqlite";
    std::string host;
    int port = 0;
    std::string database;
    std::string username;
    std::string password;
};
DatasourceConfig datasourceConfig(const std::string &dsn);

// Opens a connection to `dsn`. The datasource config (webstrada::config::
// datasources) selects the backend: a datasource configured with
// backend="mysql" connects via the MySQL driver, backend="postgres" (or
// "postgresql"/"pg") via the PostgreSQL driver; anything else uses SQLite.
// When no config entry exists, the datasource is treated as SQLite
// (DNS_<name>.sqlite), preserving the pre-config behavior.
DBConnection *openConnection(const std::string &dsn, long long timeoutMs);

// Implemented by the SQLite / MySQL / PostgreSQL backends; registers the
// built-in drivers. Called lazily by openConnection; exposed so the backends
// are pulled into the link even from a static library.
void registerBuiltinDrivers();

} // namespace db
} // namespace webstrada

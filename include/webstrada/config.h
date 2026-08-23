#pragma once

#include <map>
#include <string>

namespace webstrada {
namespace config {

// Server-wide configuration. Set at daemon startup; read by the compiler when
// constructing the AST so it can enable whitespace removal between CFML tags.
// Mirrors ColdFusion's "Enable Whitespace Management" Administrator setting,
// which is enabled by default in ColdFusion (and therefore here too).
//
// TODO: load from a config file instead of a hardcoded default.
extern bool enableWhitespaceManagement;

// Default character encoding used for page output. ColdFusion serves pages in
// UTF-8 by default; the Administrator can change this (see CF Administrator >
// Server Settings > Settings > Default Encoding). The worker initializes each
// request's output charset from this value.
//
// TODO: load from a config file / the Administrator panel instead of a
// hardcoded default.
extern std::string defaultOutputCharset;

// Default character encoding used to read template source files when no BOM,
// no <cfprocessingdirective pageEncoding> directive and no high-confidence ICU
// charset detection applies. ColdFusion's Administrator "Default Encoding"
// setting defaults to UTF-8 (see the template_reader.cpp input-encoding
// order).
//
// TODO: load from a config file / the Administrator panel instead of a
// hardcoded default.
extern std::string defaultInputCharset;

// Minimum ICU charset-detection confidence (0-100) below which the detected
// charset is ignored and the default input charset is used instead. Mirrors
// ColdFusion's coldfusion.charsetdetection.minthreshhold property (the admin
// default here is 80, i.e. a detected charset overrides the default input
// charset when detection is at least 80% confident).
extern int charsetDetectionMinConfidence;

// Filesystem path of the SQLite database that backs the APPLICATION and
// SESSION scopes. Empty (the default) means "next to the WebStrada binary":
// the worker resolves the executable path at startup and uses
// <binary-dir>/WebStrada-scopes.sqlite. SQLite is opened in WAL mode so all
// prefork worker processes share one store safely.
//
// TODO: load from a config file / the Administrator panel instead of a
// hardcoded default.
extern std::string scopeDbPath;

// Filesystem path of the SQLite database that backs the CacheGet/CachePut/
// CacheRegion* family. Empty (the default) means "next to the WebStrada
// binary": the worker resolves the executable path at startup and uses
// <binary-dir>/WebStrada-cache.sqlite. Opened in WAL mode like the scope
// store so prefork worker processes share one cache.
//
// TODO: load from a config file / the Administrator panel instead of a
// hardcoded default.
extern std::string cacheDbPath;

// Directory holding the SQLite database file of every <cfquery> datasource
// (one file per DSN: DNS_<name>.sqlite). Empty (the default) means "next to
// the WebStrada binary" (resolved from /proc/self/exe at query time). Tests
// override it to keep their DSN files out of the repo.
extern std::string dsnDbDir;

// Connection parameters of a named <cfquery> datasource. The `backend` field
// selects the driver ("sqlite" (the default), "mysql", "postgres" — the last
// also accepts "postgresql"/"pg"); for "mysql"/"postgres" the host / port /
// database / username / password fields configure the server connection. A
// datasource with no config entry is treated as a SQLite datasource
// (DNS_<name>.sqlite).
//
// TODO: load from a config file / the Administrator panel instead of being set
// programmatically at startup (worker/main) or by the test suite.
struct Datasource {
    std::string backend;      // "sqlite" (default), "mysql" or "postgres"
    std::string host;         // mysql/postgres: server host
    int port = 0;             // mysql/postgres: server port (0 = driver default)
    std::string database;     // mysql/postgres: database name
    std::string username;     // mysql/postgres: login
    std::string password;     // mysql/postgres: password
};

// Case-insensitive ordering for the datasource map, so datasource names behave
// like ColdFusion's (case-insensitive) while preserving the casing of the first
// insertion — the admin panel shows the name as it was entered.
struct CiStrLess {
    bool operator()(const std::string &a, const std::string &b) const {
        size_t n = a.size() < b.size() ? a.size() : b.size();
        for (size_t i = 0; i < n; i++) {
            char ca = a[i], cb = b[i];
            if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb - 'A' + 'a');
            if (ca != cb) return ca < cb;
        }
        return a.size() < b.size();
    }
};

extern std::map<std::string, Datasource, CiStrLess> datasources;

// Loads datasource entries from environment variables, for environments that
// do not yet have a config file. The format is a set of variables per
// datasource named <PREFIX><UPPER_DSN>_*:
//
//   WSDATASOURCE_MYDSN_BACKEND=mysql
//   WSDATASOURCE_MYDSN_HOST=127.0.0.1
//   WSDATASOURCE_MYDSN_PORT=3306
//   WSDATASOURCE_MYDSN_DATABASE=app
//   WSDATASOURCE_MYDSN_USERNAME=root
//   WSDATASOURCE_MYDSN_PASSWORD=secret
//
// where <UPPER_DSN> is the datasource name uppercased with '-' replaced by
// '_'. Any subset of the fields may be provided (only variables that exist are
// set). Call at daemon/CLI startup.
void loadDatasourcesFromEnv();

// Default lifespan of APPLICATION and SESSION scopes when <cfapplication>
// omits the applicationtimeout/sessiontimeout attributes. They mirror
// ColdFusion's Administrator defaults (2 days / 20 minutes), expressed in
// seconds.
extern double defaultApplicationTimeoutSeconds;
extern double defaultSessionTimeoutSeconds;

// Whether every executed <cfquery> is logged to stdout (datasource, name, row
// count, elapsed ms and the evaluated SQL) — plus, since openConnection wraps
// every backend connection in a LoggingConnection, every operation on the
// abstract DB layer ([db] open/execute/begin/commit/rollback/savepoint/
// rollbackTo/tableColumns/storedProc/close lines, see src/db/db.cpp). On by
// default for the daemon so the dev web server (http-dev.py) shows the queries
// each request runs; the CLI and the unit-test binary disable it because their
// stdout is a data channel (verify_with_coldfusion.py compares it
// byte-for-byte against CF).
extern bool enableQueryLogging;

// Whether the CF Administrator "Enable Debugging" setting is on. The engine
// has no debug output section, so this stays false (CF's default and the RDS
// host setting): <cftimer>'s inline/comment/outline timing display and the
// <cftrace> tag are gated on it exactly like CF's DebuggingService (when false,
// cftimer still evaluates its body and cftrace is a complete no-op).
extern bool debugEnabled;

// The CF Administrator "compile extensions for include" setting
// (`compileextforinclude`), a comma-delimited list of file extensions that
// <cfinclude> compiles and executes as CFML in addition to `.cfm`/`.cfml`.
// The wildcard `*` means every included file (`.sql`, `.txt`, `.xyz`, ...) is
// compiled; an empty value restricts compilation to `.cfm`/`.cfml` targets and
// everything else is read and output verbatim. ColdFusion's stock install
// (neo-runtime.xml) ships `*` — the RDS host keeps that default — and
// IncludeTag.checkForType matches the uppercased file name against each
// extension. MangoBlog's setup relies on this: `<cfinclude template="mysql.sql">`
// runs the <cfquery> DDL/INSERT statements that create the `blog`/`entry`/...
// tables (was broken in this engine, which hardcoded the static-include path
// for every non-CFML extension — BUGS.md "cfinclude of a .sql/.txt file").
extern std::string compileExtForInclude;

// ---------------------------------------------------------------------------
// Configuration file support.
//
// The server configuration lives in a JSON file (webstrada-config.json). The
// daemon resolves it at startup (WEBSTRADA_CONFIG overrides, else next to the
// WebStrada binary), loads it into the config::* globals above, and creates it
// from the built-in defaults when it does not exist yet. The admin panel reads
// and writes it through the __configGet()/__configSet() compiler-extension
// functions (src/cffunctions/fn_configget.cpp / fn_configset.cpp).
// ---------------------------------------------------------------------------

// Resolved path of the configuration file. Empty when config support was never
// initialized (CLI/unit-test processes that set their own values directly).
extern std::string configFilePath;

// Resolves configFilePath (env WEBSTRADA_CONFIG or binary dir), loads the file
// into the config::* globals if present, and bootstraps it from the current
// defaults when absent. Call once at daemon startup, before forking workers.
void initialize();

// True when a configuration file exists on disk (loaded at startup or
// bootstrapped from defaults).
bool hasConfigFile();

// Re-reads the configuration file into the config::* globals when its mtime
// changed since the last load, so prefork workers pick up admin-panel writes
// without a restart. Returns true when the file was re-applied. Cheap: a single
// stat() when nothing changed. Call it once per request.
bool reloadIfChanged();

// Persists the current config::* globals to configFilePath atomically (write a
// temp file in the same directory, then rename). No-op when configFilePath is
// empty (CLI/unit-test processes without config support).
void save();

// Resets every config::* global to the built-in defaults and persists them.
// Used by the __configReset() compiler-extension function ("Restore Defaults"
// in the admin panel).
void resetToDefaults();

} // namespace config
} // namespace webstrada

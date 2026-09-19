#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct sqlite3;

namespace webstrada {

// SQLite-backed persistence for the APPLICATION and SESSION scopes. The daemon
// opens one database file (default: next to the WebStrada binary) in WAL mode,
// so every prefork worker process reads/writes the same store safely.
//
// Each scope is one row holding a SerializeJSON blob plus a unix-epoch
// expiry. A row with expires_at == 0 never expires; otherwise it is live while
// expires_at > now. Expired rows are skipped by the load queries and deleted
// lazily (both on load and by purgeExpired), which implements ColdFusion's
// "purge when the timeout is reached" without a sweeper thread.
class ScopeStore
{
public:
    ScopeStore() = default;
    ~ScopeStore();

    ScopeStore(const ScopeStore &) = delete;
    ScopeStore &operator=(const ScopeStore &) = delete;

    // Creates/opens the database and its schema. Returns false on failure;
    // the failure detail is left in lastError().
    bool open(const std::string &dbPath);
    void close();

    bool isOpen() const { return m_db != nullptr; }
    const std::string &lastError() const { return m_lastError; }

    // Load a scope. Returns true and fills *data when a live (non-expired)
    // row exists. `now` is a unix-epoch seconds value supplied by the caller
    // so the store stays clock-agnostic. An expired row is deleted lazily.
    // For sessions, *startTime (nullable) receives the row's creation time.
    bool loadApplication(const std::string &appName, int64_t now, std::string &data);
    bool loadSession(const std::string &appName, const std::string &sessionId, int64_t now, std::string &data, int64_t *startTime = nullptr);

    // Persist a scope. `expiresAt` is a unix-epoch seconds value; 0 means no
    // expiry. `now` is recorded as last_access. `startTime` (0 = keep the
    // existing creation time) is recorded for sessions. `cfcPath` optionally records
    // the filesystem path to the defining Application.cfc.
    bool storeApplication(const std::string &appName, const std::string &data, int64_t expiresAt, int64_t now, const std::string &cfcPath = "");
    bool storeSession(const std::string &appName, const std::string &sessionId, const std::string &data, int64_t expiresAt, int64_t now, int64_t startTime = 0, const std::string &cfcPath = "");

    bool removeApplication(const std::string &appName);
    bool removeSession(const std::string &appName, const std::string &sessionId);

    // Rename a session row, preserving its data, expiry and creation time
    // (SessionRotate).
    bool rotateSession(const std::string &appName, const std::string &oldSessionId, const std::string &newSessionId);

    // Atomically touch/extend the lifetime of an active scope without loading or mutating data.
    // If expires_at == 0 (never expires) or expires_at > now, extends expiry to newExpiresAt
    // and updates last_access = now.
    // Returns true if a live row was touched, or false if the row is expired/missing.
    bool touchApplication(const std::string &appName, int64_t newExpiresAt, int64_t now);
    bool touchSession(const std::string &appName, const std::string &sessionId, int64_t newExpiresAt, int64_t now);

    // Query the earliest expiration across the system (lowest expires_at > 0).
    // Returns true and sets *earliestExpiresAt if an expiring row exists.
    // Returns false if no rows have an expiry scheduled.
    bool earliestExpiry(int64_t &earliestExpiresAt);

    struct ExpiredScopeRecord {
        std::string scopeKind; // "APPLICATION" or "SESSION"
        std::string appName;
        std::string scopeId;
        std::string data;
        std::string cfcPath;
        int64_t startTime = 0;
    };

    // Atomically claim (copy data and delete) all expired rows where expires_at > 0 AND expires_at <= now.
    // The SQLite write transaction is committed immediately before returning.
    bool claimExpired(int64_t now, std::vector<ExpiredScopeRecord> &outExpired);

    // Monotonic server-wide counter used to mint CFID values (ColdFusion
    // assigns CFIDs from a single server-global sequence). Returns false on
    // failure.
    bool nextCfid(int64_t &cfid);

    // Delete every row whose expires_at is in the past (and not 0).
    bool purgeExpired(int64_t now);

    // ---- <cflogin> auth pool (CF's SecurityScopeTracker.msecurityPool) ----
    //
    // The login model maps an auth token (the CFAUTHORIZATION_<apptoken>
    // cookie/session value) to a SecurityTable {username, password, roles,
    // appToken, idle timeout, last access}. CF keeps this pool in memory;
    // this engine stores it in SQLite so every prefork worker shares logins.
    // `maxInactiveMs` is the tag's idletimeout in milliseconds; a row whose
    // last_access is older than maxInactiveMs is skipped by the load query and
    // deleted lazily (CF's SecurityCleanUpAgent, running every 10 s).

    // Load a live auth-table row by token. Returns true and fills *out
    // (username, password, roles joined by '\r', appToken, maxInactiveMs)
    // when the token exists and is not idle-expired. `nowMs` is a unix-epoch
    // milliseconds value. An idle-expired row is deleted lazily.
    bool loadSecurity(const std::string &token, int64_t nowMs,
                      std::string &username, std::string &password,
                      std::string &roles, std::string &appToken,
                      int64_t &maxInactiveMs);

    // Persist (or update) an auth-table row. `lastAccessMs` is recorded as
    // the row's last_access. `maxInactiveMs` 0 = no idle expiry (never
    // auto-purged; CF's default when the table has no timeout).
    bool storeSecurity(const std::string &token, const std::string &username,
                       const std::string &password, const std::string &roles,
                       const std::string &appToken, int64_t maxInactiveMs,
                       int64_t lastAccessMs);

    // Remove one auth-table row by token (logout current) or every row whose
    // app_token matches (logout all / others — `matchAppToken`).
    bool removeSecurity(const std::string &token);
    bool removeSecurityByAppToken(const std::string &appToken);

private:
    bool exec(const char *sql);
    bool executeDelete(const char *sql, const std::string &a, const std::string &b, const std::string &c);

    sqlite3 *m_db = nullptr;
    std::string m_lastError;
};

} // namespace webstrada

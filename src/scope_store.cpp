#include <webstrada/scope_store.h>

#include <sqlite3.h>

#include <cstring>

namespace webstrada {

ScopeStore::~ScopeStore()
{
    close();
}

bool ScopeStore::exec(const char *sql)
{
    char *err = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        m_lastError = err ? err : "sqlite3_exec failed";
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool ScopeStore::open(const std::string &dbPath)
{
    if (m_db) close();

    if (sqlite3_open(dbPath.c_str(), &m_db) != SQLITE_OK) {
        m_lastError = m_db ? sqlite3_errmsg(m_db) : "sqlite3_open failed";
        if (m_db) sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // WAL so prefork workers can read while another writes; busy_timeout so a
    // transient single-writer lock just queues instead of erroring. Per the
    // task direction we rely on SQLite's own write serialization rather than
    // adding application-level locking.
    if (!exec("PRAGMA journal_mode=WAL;")) return false;
    if (!exec("PRAGMA busy_timeout=5000;")) return false;

    static const char *kSchema =
        "CREATE TABLE IF NOT EXISTS cf_scope ("
        " scope_kind  TEXT NOT NULL,"
        " app_name    TEXT NOT NULL,"
        " scope_id    TEXT NOT NULL DEFAULT '',"
        " data        TEXT NOT NULL,"
        " expires_at  INTEGER NOT NULL DEFAULT 0,"
        " last_access INTEGER NOT NULL,"
        " start_time  INTEGER NOT NULL DEFAULT 0,"
        " PRIMARY KEY (scope_kind, app_name, scope_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS cf_seq ("
        " seq_key TEXT PRIMARY KEY,"
        " next_val INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS cf_security ("
        " token          TEXT PRIMARY KEY,"
        " username       TEXT NOT NULL,"
        " password       TEXT NOT NULL,"
        " roles          TEXT NOT NULL,"
        " app_token      TEXT NOT NULL,"
        " max_inactive   INTEGER NOT NULL DEFAULT 0,"
        " last_access    INTEGER NOT NULL"
        ");";
    if (!exec(kSchema)) return false;

    // Migration for stores created before the start_time column existed.
    exec("ALTER TABLE cf_scope ADD COLUMN start_time INTEGER NOT NULL DEFAULT 0;");

    return true;
}

void ScopeStore::close()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

// Load a live scope row: scope_kind, app_name, scope_id, now.
static bool loadRow(sqlite3 *db, std::string &lastError,
                    const char *kind, const std::string &appName, const std::string &scopeId,
                    int64_t now, std::string &data, int64_t *startTime)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT data, start_time FROM cf_scope"
                      " WHERE scope_kind=? AND app_name=? AND scope_id=?"
                      "   AND (expires_at = 0 OR expires_at > ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, kind, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, appName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, scopeId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, now);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        data.assign(reinterpret_cast<const char *>(text),
                    text ? static_cast<size_t>(sqlite3_column_bytes(stmt, 0)) : 0);
        if (startTime) *startTime = sqlite3_column_int64(stmt, 1);
        found = true;
    }
    int rc = sqlite3_finalize(stmt);
    if (rc != SQLITE_OK) {
        lastError = sqlite3_errmsg(db);
        return false;
    }

    if (found) {
        // Touch last_access so the most recent use is recorded.
        sqlite3_stmt *upd = nullptr;
        const char *updSql = "UPDATE cf_scope SET last_access=? WHERE scope_kind=? AND app_name=? AND scope_id=?;";
        if (sqlite3_prepare_v2(db, updSql, -1, &upd, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(upd, 1, now);
            sqlite3_bind_text(upd, 2, kind, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(upd, 3, appName.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(upd, 4, scopeId.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(upd);
            sqlite3_finalize(upd);
        }
    } else {
        // Lazy purge of the expired row.
        sqlite3_stmt *del = nullptr;
        const char *delSql = "DELETE FROM cf_scope WHERE scope_kind=? AND app_name=? AND scope_id=?"
                             " AND expires_at > 0 AND expires_at <= ?;";
        if (sqlite3_prepare_v2(db, delSql, -1, &del, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(del, 1, kind, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(del, 2, appName.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(del, 3, scopeId.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(del, 4, now);
            sqlite3_step(del);
            sqlite3_finalize(del);
        }
    }
    return found;
}

bool ScopeStore::loadApplication(const std::string &appName, int64_t now, std::string &data)
{
    return loadRow(m_db, m_lastError, "APPLICATION", appName, "", now, data, nullptr);
}

bool ScopeStore::loadSession(const std::string &appName, const std::string &sessionId, int64_t now, std::string &data, int64_t *startTime)
{
    return loadRow(m_db, m_lastError, "SESSION", appName, sessionId, now, data, startTime);
}

// Upsert one scope row.
static bool storeRow(sqlite3 *db, std::string &lastError,
                     const char *kind, const std::string &appName, const std::string &scopeId,
                     const std::string &data, int64_t expiresAt, int64_t now, int64_t startTime)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO cf_scope (scope_kind, app_name, scope_id, data, expires_at, last_access, start_time)"
                      " VALUES (?,?,?,?,?,?,?)"
                      " ON CONFLICT(scope_kind, app_name, scope_id)"
                      " DO UPDATE SET data=excluded.data, expires_at=excluded.expires_at,"
                      "               last_access=excluded.last_access,"
                      "               start_time=CASE WHEN excluded.start_time > 0 THEN excluded.start_time ELSE cf_scope.start_time END;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, kind, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, appName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, scopeId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, data.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, expiresAt);
    sqlite3_bind_int64(stmt, 6, now);
    sqlite3_bind_int64(stmt, 7, startTime);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        lastError = sqlite3_errmsg(db);
        return false;
    }
    return true;
}

bool ScopeStore::storeApplication(const std::string &appName, const std::string &data, int64_t expiresAt, int64_t now)
{
    return storeRow(m_db, m_lastError, "APPLICATION", appName, "", data, expiresAt, now, 0);
}

bool ScopeStore::storeSession(const std::string &appName, const std::string &sessionId, const std::string &data, int64_t expiresAt, int64_t now, int64_t startTime)
{
    return storeRow(m_db, m_lastError, "SESSION", appName, sessionId, data, expiresAt, now, startTime);
}

bool ScopeStore::removeApplication(const std::string &appName)
{
    // ApplicationStop removes the application scope and all of its sessions.
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM cf_scope WHERE app_name=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, appName.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

bool ScopeStore::removeSession(const std::string &appName, const std::string &sessionId)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM cf_scope WHERE scope_kind='SESSION' AND app_name=? AND scope_id=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, appName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

bool ScopeStore::rotateSession(const std::string &appName, const std::string &oldSessionId, const std::string &newSessionId)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "UPDATE cf_scope SET scope_id=? WHERE scope_kind='SESSION' AND app_name=? AND scope_id=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, newSessionId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, appName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, oldSessionId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

bool ScopeStore::nextCfid(int64_t &cfid)
{
    // BEGIN IMMEDIATE serializes writers, so concurrent workers never hand out
    // the same CFID. The critical section is a single tiny read+write.
    if (!exec("BEGIN IMMEDIATE;")) return false;

    sqlite3_stmt *sel = nullptr;
    int64_t next = 1;
    bool ok = true;
    if (sqlite3_prepare_v2(m_db, "SELECT next_val FROM cf_seq WHERE seq_key='cfid';", -1, &sel, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        ok = false;
    } else {
        if (sqlite3_step(sel) == SQLITE_ROW) next = sqlite3_column_int64(sel, 0);
        sqlite3_finalize(sel);
    }

    if (ok) {
        sqlite3_stmt *upd = nullptr;
        if (sqlite3_prepare_v2(m_db,
                "INSERT INTO cf_seq (seq_key, next_val) VALUES ('cfid', ?)"
                " ON CONFLICT(seq_key) DO UPDATE SET next_val=excluded.next_val;",
                -1, &upd, nullptr) != SQLITE_OK) {
            m_lastError = sqlite3_errmsg(m_db);
            ok = false;
        } else {
            sqlite3_bind_int64(upd, 1, next + 1);
            if (sqlite3_step(upd) != SQLITE_DONE) {
                m_lastError = sqlite3_errmsg(m_db);
                ok = false;
            }
            sqlite3_finalize(upd);
        }
    }

    sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
    if (ok) cfid = next;
    return ok;
}

bool ScopeStore::purgeExpired(int64_t now)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM cf_scope WHERE expires_at > 0 AND expires_at <= ?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_int64(stmt, 1, now);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

bool ScopeStore::loadSecurity(const std::string &token, int64_t nowMs,
                              std::string &username, std::string &password,
                              std::string &roles, std::string &appToken,
                              int64_t &maxInactiveMs)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT username, password, roles, app_token, max_inactive"
                      " FROM cf_security WHERE token=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *t;
        t = sqlite3_column_text(stmt, 0);
        username.assign(reinterpret_cast<const char *>(t), t ? static_cast<size_t>(sqlite3_column_bytes(stmt, 0)) : 0);
        t = sqlite3_column_text(stmt, 1);
        password.assign(reinterpret_cast<const char *>(t), t ? static_cast<size_t>(sqlite3_column_bytes(stmt, 0)) : 0);
        t = sqlite3_column_text(stmt, 2);
        roles.assign(reinterpret_cast<const char *>(t), t ? static_cast<size_t>(sqlite3_column_bytes(stmt, 0)) : 0);
        t = sqlite3_column_text(stmt, 3);
        appToken.assign(reinterpret_cast<const char *>(t), t ? static_cast<size_t>(sqlite3_column_bytes(stmt, 0)) : 0);
        maxInactiveMs = sqlite3_column_int64(stmt, 4);
        int64_t lastAccess = 0;
        {
            // Re-read last_access from the same row via a second statement is
            // unnecessary: compute the idle check from maxInactiveMs + nowMs
            // after fetching the row's last_access. Since the SELECT above did
            // not project it, re-prepare a tiny query.
            sqlite3_stmt *s2 = nullptr;
            if (sqlite3_prepare_v2(m_db, "SELECT last_access FROM cf_security WHERE token=?;", -1, &s2, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(s2, 1, token.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(s2) == SQLITE_ROW) lastAccess = sqlite3_column_int64(s2, 0);
                sqlite3_finalize(s2);
            }
        }
        // Idle expiry: CF's SecurityCleanUpAgent removes a table whose
        // last_access is older than maxInactiveMs. 0 = never expires.
        if (maxInactiveMs > 0 && (nowMs - lastAccess) > maxInactiveMs) {
            found = false;
        } else {
            found = true;
            // Touch last_access (CF's getSecurity does setLastAccess(now)).
            sqlite3_stmt *upd = nullptr;
            if (sqlite3_prepare_v2(m_db, "UPDATE cf_security SET last_access=? WHERE token=?;", -1, &upd, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(upd, 1, nowMs);
                sqlite3_bind_text(upd, 2, token.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(upd);
                sqlite3_finalize(upd);
            }
        }
    }
    int rc = sqlite3_finalize(stmt);
    if (rc != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    if (!found) {
        // Lazy purge of the idle-expired row.
        sqlite3_stmt *del = nullptr;
        const char *delSql = "DELETE FROM cf_security WHERE token=?;";
        if (sqlite3_prepare_v2(m_db, delSql, -1, &del, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(del, 1, token.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(del);
            sqlite3_finalize(del);
        }
    }
    return found;
}

bool ScopeStore::storeSecurity(const std::string &token, const std::string &username,
                               const std::string &password, const std::string &roles,
                               const std::string &appToken, int64_t maxInactiveMs,
                               int64_t lastAccessMs)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO cf_security (token, username, password, roles, app_token, max_inactive, last_access)"
                      " VALUES (?,?,?,?,?,?,?)"
                      " ON CONFLICT(token) DO UPDATE SET username=excluded.username,"
                      " password=excluded.password, roles=excluded.roles,"
                      " app_token=excluded.app_token, max_inactive=excluded.max_inactive,"
                      " last_access=excluded.last_access;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, roles.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, appToken.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, maxInactiveMs);
    sqlite3_bind_int64(stmt, 7, lastAccessMs);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

bool ScopeStore::removeSecurity(const std::string &token)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM cf_security WHERE token=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

bool ScopeStore::removeSecurityByAppToken(const std::string &appToken)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM cf_security WHERE app_token=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, appToken.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

} // namespace webstrada

#include <webstrada/cache_store.h>
#include <webstrada/config.h>

#include <sqlite3.h>

#include <cstring>
#include <unistd.h>

namespace webstrada {

namespace {

CacheStore g_cacheStore;

} // namespace

CacheStore &cache_store()
{
    return g_cacheStore;
}

void open_cache_store()
{
    std::string dbPath = config::cacheDbPath;
    if (dbPath.empty()) {
        char exe[4096];
        ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (n > 0) {
            exe[n] = '\0';
            std::string path(exe);
            size_t slash = path.find_last_of('/');
            dbPath = (slash != std::string::npos)
                ? path.substr(0, slash + 1) + "WebStrada-cache.sqlite"
                : "WebStrada-cache.sqlite";
        } else {
            dbPath = "WebStrada-cache.sqlite";
        }
    }
    if (!g_cacheStore.open(dbPath)) {
        fprintf(stderr, "[WebStrada] Warning: could not open cache database %s: %s\n",
                dbPath.c_str(), g_cacheStore.lastError().c_str());
    }
}

CacheStore::~CacheStore()
{
    close();
}

bool CacheStore::exec(const char *sql)
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

bool CacheStore::isStandardRegion(const std::string &region) const
{
    if (region.size() != 6 && region.size() != 5) return false; // OBJECT(6)/QUERY(5)/TEMPLATE(8)
    if (region == "OBJECT" || region == "QUERY" || region == "TEMPLATE") return true;
    return false;
}

bool CacheStore::open(const std::string &dbPath)
{
    if (m_db) close();

    if (sqlite3_open(dbPath.c_str(), &m_db) != SQLITE_OK) {
        m_lastError = m_db ? sqlite3_errmsg(m_db) : "sqlite3_open failed";
        if (m_db) sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    if (!exec("PRAGMA journal_mode=WAL;")) return false;
    if (!exec("PRAGMA busy_timeout=5000;")) return false;

    static const char *kSchema =
        "CREATE TABLE IF NOT EXISTS cf_cache_region ("
        " region TEXT PRIMARY KEY,"
        " props  TEXT NOT NULL DEFAULT '{}'"
        ");"
        "CREATE TABLE IF NOT EXISTS cf_cache ("
        " region      TEXT NOT NULL,"
        " id          TEXT NOT NULL,"
        " value       TEXT NOT NULL,"
        " timetolive  INTEGER NOT NULL DEFAULT 0,"
        " timetoidle  INTEGER NOT NULL DEFAULT 0,"
        " created     INTEGER NOT NULL,"
        " lastaccess  INTEGER NOT NULL,"
        " lastupdate  INTEGER NOT NULL,"
        " hits        INTEGER NOT NULL DEFAULT 0,"
        " PRIMARY KEY (region, id)"
        ");";
    if (!exec(kSchema)) return false;

    // Standard CF regions always exist.
    exec("INSERT OR IGNORE INTO cf_cache_region (region, props) VALUES ('OBJECT', '{}');");
    exec("INSERT OR IGNORE INTO cf_cache_region (region, props) VALUES ('TEMPLATE', '{}');");
    exec("INSERT OR IGNORE INTO cf_cache_region (region, props) VALUES ('QUERY', '{}');");

    return true;
}

void CacheStore::close()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool CacheStore::regionExists(const std::string &region)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT 1 FROM cf_cache_region WHERE region=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    bool found = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return found;
}

bool CacheStore::createRegion(const std::string &region)
{
    if (regionExists(region)) return false;
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO cf_cache_region (region, props) VALUES (?, '{}');";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

bool CacheStore::removeRegion(const std::string &region)
{
    if (!regionExists(region)) return false;
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM cf_cache_region WHERE region=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Drop the region's entries too.
    sqlite3_stmt *del = nullptr;
    const char *delSql = "DELETE FROM cf_cache WHERE region=?;";
    if (sqlite3_prepare_v2(m_db, delSql, -1, &del, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(del, 1, region.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(del);
        sqlite3_finalize(del);
    }
    return true;
}

std::vector<std::string> CacheStore::regionList()
{
    std::vector<std::string> out;
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT region FROM cf_cache_region ORDER BY region;", -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return out;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *t = sqlite3_column_text(stmt, 0);
        if (t) out.emplace_back(reinterpret_cast<const char *>(t));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::string CacheStore::regionProperties(const std::string &region)
{
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT props FROM cf_cache_region WHERE region=?;", -1, &stmt, nullptr) != SQLITE_OK) {
        return "{}";
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    std::string props = "{}";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *t = sqlite3_column_text(stmt, 0);
        if (t) props.assign(reinterpret_cast<const char *>(t), static_cast<size_t>(sqlite3_column_bytes(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return props;
}

void CacheStore::setRegionProperties(const std::string &region, const std::string &propsJson)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO cf_cache_region (region, props) VALUES (?, ?)"
                      " ON CONFLICT(region) DO UPDATE SET props=excluded.props;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, propsJson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool CacheStore::put(const std::string &region, const std::string &id, const std::string &valueJson,
                     int64_t timetoliveMs, int64_t timetoidleMs, int64_t nowMs)
{
    // Ensure the region exists (CF auto-creates on put when absent).
    sqlite3_stmt *reg = nullptr;
    if (sqlite3_prepare_v2(m_db, "INSERT OR IGNORE INTO cf_cache_region (region, props) VALUES (?, '{}');",
                           -1, &reg, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(reg, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(reg);
    sqlite3_finalize(reg);

    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO cf_cache (region, id, value, timetolive, timetoidle, created, lastaccess, lastupdate, hits)"
                      " VALUES (?,?,?,?,?,?,?,?,0)"
                      " ON CONFLICT(region, id) DO UPDATE SET value=excluded.value, timetolive=excluded.timetolive,"
                      " timetoidle=excluded.timetoidle, created=excluded.created, lastaccess=excluded.lastaccess,"
                      " lastupdate=excluded.lastupdate, hits=0;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, valueJson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, timetoliveMs);
    sqlite3_bind_int64(stmt, 5, timetoidleMs);
    sqlite3_bind_int64(stmt, 6, nowMs);
    sqlite3_bind_int64(stmt, 7, nowMs);
    sqlite3_bind_int64(stmt, 8, nowMs);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

bool CacheStore::get(const std::string &region, const std::string &id, int64_t nowMs,
                     std::string &valueJson, bool quiet)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT value, timetolive, timetoidle, created, lastaccess FROM cf_cache"
                      " WHERE region=? AND id=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t ttl = sqlite3_column_int64(stmt, 1);
        int64_t idle = sqlite3_column_int64(stmt, 2);
        int64_t created = sqlite3_column_int64(stmt, 3);
        int64_t lastAccess = sqlite3_column_int64(stmt, 4);
        bool expired = (ttl > 0 && created + ttl <= nowMs) || (idle > 0 && lastAccess + idle <= nowMs);
        if (!expired) {
            const unsigned char *t = sqlite3_column_text(stmt, 0);
            valueJson.assign(reinterpret_cast<const char *>(t), static_cast<size_t>(sqlite3_column_bytes(stmt, 0)));
            found = true;
        }
    }
    sqlite3_finalize(stmt);

    if (found) {
        if (!quiet) {
            // Update hits and last_access.
            sqlite3_stmt *upd = nullptr;
            const char *updSql = "UPDATE cf_cache SET hits=hits+1, lastaccess=? WHERE region=? AND id=?;";
            if (sqlite3_prepare_v2(m_db, updSql, -1, &upd, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(upd, 1, nowMs);
                sqlite3_bind_text(upd, 2, region.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(upd, 3, id.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(upd);
                sqlite3_finalize(upd);
            }
        }
    } else {
        // Lazy purge of an expired row.
        sqlite3_stmt *del = nullptr;
        const char *delSql = "DELETE FROM cf_cache WHERE region=? AND id=? AND ("
                             " (timetolive > 0 AND created + timetolive <= ?) OR"
                             " (timetoidle > 0 AND lastaccess + timetoidle <= ?));";
        if (sqlite3_prepare_v2(m_db, delSql, -1, &del, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(del, 1, region.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(del, 2, id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(del, 3, nowMs);
            sqlite3_bind_int64(del, 4, nowMs);
            sqlite3_step(del);
            sqlite3_finalize(del);
        }
    }
    return found;
}

bool CacheStore::idExists(const std::string &region, const std::string &id, int64_t nowMs)
{
    std::string value;
    return get(region, id, nowMs, value, /*quiet=*/true);
}

bool CacheStore::remove(const std::string &region, const std::string &id)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM cf_cache WHERE region=? AND id=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

void CacheStore::removeAll(const std::string &region)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "DELETE FROM cf_cache WHERE region=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<std::string> CacheStore::getAllIds(const std::string &region, int64_t nowMs, bool includeExpired)
{
    std::vector<std::string> out;
    std::string sql;
    if (includeExpired) {
        sql = "SELECT id FROM cf_cache WHERE region=? ORDER BY created;";
    } else {
        sql = "SELECT id FROM cf_cache WHERE region=? AND"
              " NOT (timetolive > 0 AND created + timetolive <= ?)"
              " AND NOT (timetoidle > 0 AND lastaccess + timetoidle <= ?)"
              " ORDER BY created;";
    }
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return out;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    if (!includeExpired) {
        sqlite3_bind_int64(stmt, 2, nowMs);
        sqlite3_bind_int64(stmt, 3, nowMs);
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *t = sqlite3_column_text(stmt, 0);
        if (t) out.emplace_back(reinterpret_cast<const char *>(t));
    }
    sqlite3_finalize(stmt);
    return out;
}

CacheStore::EntryMeta CacheStore::metadata(const std::string &region, const std::string &id, int64_t nowMs)
{
    EntryMeta meta;
    meta.name = region;
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT value, timetolive, timetoidle, hits, created, lastaccess, lastupdate FROM cf_cache"
                      " WHERE region=? AND id=?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return meta;
    }
    sqlite3_bind_text(stmt, 1, region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t ttl = sqlite3_column_int64(stmt, 1);
        int64_t idle = sqlite3_column_int64(stmt, 2);
        int64_t created = sqlite3_column_int64(stmt, 4);
        int64_t lastAccess = sqlite3_column_int64(stmt, 5);
        bool expired = (ttl > 0 && created + ttl <= nowMs) || (idle > 0 && lastAccess + idle <= nowMs);
        if (!expired) {
            meta.found = true;
            meta.timetolive = ttl / 1000;
            meta.timetoidle = idle / 1000;
            meta.hits = sqlite3_column_int64(stmt, 3);
            meta.createdMs = created;
            meta.lastAccessMs = lastAccess;
            meta.lastUpdateMs = sqlite3_column_int64(stmt, 6);
            meta.size = sqlite3_column_bytes(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return meta;
}

} // namespace webstrada

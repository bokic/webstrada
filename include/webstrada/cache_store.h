#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct sqlite3;

namespace webstrada {

// SQLite-backed persistent cache for the CacheGet/CachePut/CacheRegion* family
// (mirrors the ScopeStore pattern). The daemon opens one database file in WAL
// mode at startup (default: next to the WebStrada binary, configurable via
// webstrada::config::cacheDbPath), so every prefork worker process shares the
// same store safely.
//
// Each cached object is one row keyed by (region, id) holding a SerializeJSON
// blob plus the timeToLive / timeToIdle intervals in seconds and access
// statistics. An entry with timetolive == 0 AND timetoidle == 0 is eternal;
// otherwise it is live while (created + timetolive*1000 > now) AND
// (last_access + timetoidle*1000 > now). Expired rows are skipped by reads and
// deleted lazily, implementing ColdFusion's "purge when the timeout is reached"
// without a sweeper thread.
class CacheStore
{
public:
    CacheStore() = default;
    ~CacheStore();

    CacheStore(const CacheStore &) = delete;
    CacheStore &operator=(const CacheStore &) = delete;

    bool open(const std::string &dbPath);
    void close();

    bool isOpen() const { return m_db != nullptr; }
    const std::string &lastError() const { return m_lastError; }

    // Region existence / creation / removal. Standard CF regions (OBJECT,
    // TEMPLATE, QUERY) are always implicitly present; custom regions are
    // created by cacheRegionNew and removed by cacheRegionRemove.
    bool regionExists(const std::string &region);
    bool createRegion(const std::string &region);       // false if it exists
    bool removeRegion(const std::string &region);       // false if it does not exist
    std::vector<std::string> regionList();

    // Properties of a region (CacheGetProperties/CacheSetProperties). Stored
    // as a SerializeJSON blob; unknown keys round-trip untouched.
    std::string regionProperties(const std::string &region);
    void setRegionProperties(const std::string &region, const std::string &propsJson);

    // Put an object. timetolive/timetoidle are in seconds; 0+0 means eternal.
    // `now` is a unix-epoch milliseconds value. Creates the region implicitly
    // if it does not exist (CF's object cache auto-creates its region).
    bool put(const std::string &region, const std::string &id, const std::string &valueJson,
             int64_t timetoliveMs, int64_t timetoidleMs, int64_t nowMs);

    // Get an object's value JSON. Returns false when the id is absent or the
    // entry has expired. `quiet` skips the hit-count / last-access update
    // (CF's getQuiet, used by cacheIdExists / cacheGetMetadata).
    bool get(const std::string &region, const std::string &id, int64_t nowMs,
             std::string &valueJson, bool quiet);

    // Exact-id presence check (does not create the region).
    bool idExists(const std::string &region, const std::string &id, int64_t nowMs);

    // Remove one entry. Returns false when the id is absent.
    bool remove(const std::string &region, const std::string &id);

    // Remove every entry in the region (the region itself stays).
    void removeAll(const std::string &region);

    // All live (or, with includeExpired, all) ids in the region, in
    // insertion order.
    std::vector<std::string> getAllIds(const std::string &region, int64_t nowMs, bool includeExpired);

    // Per-entry metadata for CacheGetMetadata. `found` is false for an absent
    // or expired id.
    struct EntryMeta {
        bool found = false;
        std::string name;          // the region name
        int64_t timetolive = 0;    // stored seconds (0 = eternal)
        int64_t timetoidle = 0;    // stored seconds
        int64_t hits = 0;
        int64_t createdMs = 0;
        int64_t lastAccessMs = 0;
        int64_t lastUpdateMs = 0;
        int64_t size = 0;          // byte length of the stored value
    };
    EntryMeta metadata(const std::string &region, const std::string &id, int64_t nowMs);

private:
    bool exec(const char *sql);
    bool isStandardRegion(const std::string &region) const;

    sqlite3 *m_db = nullptr;
    std::string m_lastError;
};

// Process-wide cache store accessor. The worker opens it at startup
// (open_cache_store); the CLI and unit tests open it directly. Returns the
// process-global CacheStore instance.
CacheStore &cache_store();

// Resolve the cache database path (config::cacheDbPath, default next to the
// executable) and open the process-global store. Non-fatal on failure: the
// store simply stays closed and cache functions throw a clear error.
void open_cache_store();

} // namespace webstrada

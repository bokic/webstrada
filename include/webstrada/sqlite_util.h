#pragma once

#include <sqlite3.h>

#include <string>

namespace webstrada {

// Switch a database connection to WAL mode with a bounded retry. The one-time
// conversion of a fresh database file momentarily needs the write lock and can
// return SQLITE_BUSY when several prefork workers open the same system database
// at once (PRAGMA journal_mode's busy handling does not honor busy_timeout).
// Without the retry, a losing worker fails to open its store and the store
// silently stays closed for that worker's life.
inline bool sqlite_enable_wal(sqlite3 *db, std::string &lastError,
                              int maxRetries = 20, int sleepMs = 50)
{
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        char *err = nullptr;
        int rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err);
        if (rc == SQLITE_OK) return true;
        if (rc == SQLITE_BUSY) {
            if (err) sqlite3_free(err);
            sqlite3_sleep(sleepMs);
            continue;
        }
        lastError = err ? err : "PRAGMA journal_mode=WAL failed";
        if (err) sqlite3_free(err);
        return false;
    }
    lastError = "PRAGMA journal_mode=WAL failed: database is locked";
    return false;
}

} // namespace webstrada
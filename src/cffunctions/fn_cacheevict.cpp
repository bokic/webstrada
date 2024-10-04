/**
 * @file fn_cacheevict.cpp
 * @brief Compiler-extension __cacheEvict() built-in.
 *
 * Removes a single entry from the cache store: __cacheEvict(region, id).
 * Returns { ok: true } on success or { ok: false, error } when the entry is
 * absent. Backs the admin panel's per-row "Evict" button.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cache_store.h>

#include <chrono>
#include <string>

namespace cfml {

cfvariant *cf___cacheevict(const cfvariant **args, int argc)
{
    cfvariant root(cfvariant::Struct);
    if (argc < 2 || !args || !args[0] || !args[1]) {
        cfvariant ok(cfvariant::Boolean);
        ok.m_bool = false;
        root.structSet("ok", ok);
        root.structSet("error", cfvariant("__cacheEvict requires a region and an id."));
        return new cfvariant(root);
    }

    std::string region = toStdString(args[0]);
    std::string id = toStdString(args[1]);
    webstrada::CacheStore &store = webstrada::cache_store();
    int64_t nowMs = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                             std::chrono::system_clock::now().time_since_epoch())
                                             .count());

    // CacheStore::remove returns true as long as the DELETE succeeded (even
    // with zero rows), so check presence first for a truthful result.
    bool exists = store.idExists(region, id, nowMs);
    bool ok = exists && store.remove(region, id);

    cfvariant okVal(cfvariant::Boolean);
    okVal.m_bool = ok;
    root.structSet("ok", okVal);
    if (!ok) {
        root.structSet("error", cfvariant("Cache entry not found."));
    }
    return new cfvariant(root);
}

} // namespace cfml

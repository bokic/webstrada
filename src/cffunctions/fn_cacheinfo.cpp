/**
 * @file fn_cacheinfo.cpp
 * @brief Compiler-extension __cacheInfo() built-in.
 *
 * Returns a listing of every live cache entry in the SQLite-backed cache store
 * (the CacheGet / CachePut / CacheRegion* / cfcache family) plus aggregate
 * stats:
 *
 *   { entries: [ { region, id, hits, size, createdMs, lastAccessMs,
 *                  lastUpdateMs, expiresMs (0 = eternal) } ],
 *     totalEntries, totalHits, totalSize }
 *
 * Backs the admin panel's "Cache" page.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cache_store.h>
#include <webstrada/template_cache.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace cfml {

namespace {

const std::vector<std::string> kStandardRegions = {"OBJECT", "TEMPLATE", "QUERY"};

int64_t nowMs()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch())
                                    .count());
}

} // namespace

cfvariant *cf___cacheinfo(const cfvariant **args, int argc)
{
    (void)args;
    (void)argc;

    webstrada::CacheStore &store = webstrada::cache_store();
    int64_t now = nowMs();

    // All regions: the standard OBJECT / TEMPLATE / QUERY plus any explicitly
    // created custom regions.
    std::vector<std::string> regions = kStandardRegions;
    for (const auto &r : store.regionList()) {
        bool dup = false;
        for (const auto &e : regions) {
            if (e == r) { dup = true; break; }
        }
        if (!dup) regions.push_back(r);
    }

    cfvariant entries(cfvariant::Array);
    int64_t totalHits = 0;
    int64_t totalSize = 0;

    for (const auto &region : regions) {
        for (const auto &id : store.getAllIds(region, now, false)) {
            webstrada::CacheStore::EntryMeta meta = store.metadata(region, id, now);
            if (!meta.found) continue;

            cfvariant row(cfvariant::Struct);
            row.structSet("region", cfvariant(region.c_str()));
            row.structSet("id", cfvariant(id.c_str()));

            cfvariant hits(cfvariant::Long);
            hits.m_long = meta.hits;
            row.structSet("hits", hits);

            cfvariant size(cfvariant::Long);
            size.m_long = meta.size;
            row.structSet("size", size);

            cfvariant created(cfvariant::Long);
            created.m_long = meta.createdMs;
            row.structSet("createdMs", created);

            cfvariant lastAccess(cfvariant::Long);
            lastAccess.m_long = meta.lastAccessMs;
            row.structSet("lastAccessMs", lastAccess);

            cfvariant lastUpdate(cfvariant::Long);
            lastUpdate.m_long = meta.lastUpdateMs;
            row.structSet("lastUpdateMs", lastUpdate);

            // Expiry = the earliest live deadline (0 = eternal).
            int64_t expires = 0;
            if (meta.timetolive > 0) expires = meta.createdMs + meta.timetolive * 1000;
            if (meta.timetoidle > 0) {
                int64_t idleDeadline = meta.lastAccessMs + meta.timetoidle * 1000;
                if (expires == 0 || idleDeadline < expires) expires = idleDeadline;
            }
            cfvariant exp(cfvariant::Long);
            exp.m_long = expires;
            row.structSet("expiresMs", exp);

            entries.m_array->push_back(row);
            totalHits += meta.hits;
            totalSize += meta.size;
        }
    }

    cfvariant root(cfvariant::Struct);
    root.structSet("entries", entries);

    cfvariant te(cfvariant::Long);
    te.m_long = static_cast<int64_t>(entries.m_array->size());
    root.structSet("totalEntries", te);

    cfvariant th(cfvariant::Long);
    th.m_long = totalHits;
    root.structSet("totalHits", th);

    cfvariant ts(cfvariant::Long);
    ts.m_long = totalSize;
    root.structSet("totalSize", ts);

    // JIT-compiled templates (.cfm) and components (.cfc) held by this
    // worker's in-memory TemplateCache (separate from the SQLite store above).
    int compiledTemplates = 0;
    int compiledComponents = 0;
    webstrada::compiled_cache_counts(compiledTemplates, compiledComponents);

    cfvariant ct(cfvariant::Long);
    ct.m_long = compiledTemplates;
    root.structSet("compiledTemplates", ct);

    cfvariant cc(cfvariant::Long);
    cc.m_long = compiledComponents;
    root.structSet("compiledComponents", cc);

    return new cfvariant(root);
}

} // namespace cfml

/**
 * @file fn_cacheclear.cpp
 * @brief Compiler-extension __cacheClear() built-in.
 *
 * Removes every entry from the cache store across all regions (the standard
 * OBJECT / TEMPLATE / QUERY regions plus any custom ones; the regions
 * themselves stay). Returns { ok: true }. Backs the admin panel's "Clear All
 * Caches" button.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cache_store.h>

#include <string>
#include <vector>

namespace cfml {

cfvariant *cf___cacheclear(const cfvariant **args, int argc)
{
    (void)args;
    (void)argc;

    webstrada::CacheStore &store = webstrada::cache_store();
    const std::vector<std::string> standard = {"OBJECT", "TEMPLATE", "QUERY"};
    for (const auto &region : standard) store.removeAll(region);
    for (const auto &region : store.regionList()) store.removeAll(region);

    cfvariant root(cfvariant::Struct);
    cfvariant ok(cfvariant::Boolean);
    ok.m_bool = true;
    root.structSet("ok", ok);
    return new cfvariant(root);
}

} // namespace cfml

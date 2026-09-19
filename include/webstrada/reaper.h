#pragma once

#include "scope_store.h"
#include "template_cache.h"
#include <cstdint>
#include <string>

namespace webstrada {

// Atomically claims all expired session and application records from `store` (with
// data copied to memory and deleted from SQLite before returning).
// Then, in memory outside the DB transaction:
//  - For each expired session: deserializes SessionScope, retrieves ApplicationScope,
//    and invokes onSessionEnd(SessionScope, ApplicationScope) on the Application.cfc
//    (if cfcPath is set and exists).
//  - For each expired application: deserializes ApplicationScope, and invokes
//    onApplicationEnd(ApplicationScope) on the Application.cfc (if cfcPath is set).
// Standard output is discarded, and exceptions are logged without terminating execution.
// Returns the number of expired records processed.
size_t reap_expired_scopes(ScopeStore &store, int64_t now, TemplateCache *templates = nullptr);

// Main entry point for the dedicated Reaper process.
// Opens ScopeStore at `scopeDbPath`, runs the timerfd & signalfd poll loop,
// and exits cleanly with 0 on SIGINT/SIGTERM.
int run_reaper_process(const std::string &scopeDbPath = "");

// Resolves the default scope DB path ("WebStrada-scopes.sqlite" next to the binary,
// or config::scopeDbPath if non-empty).
std::string resolve_scope_db_path();

} // namespace webstrada

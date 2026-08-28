/**
 * @file fn_serverinfo.cpp
 * @brief Compiler-extension __serverInfo() built-in.
 *
 * Returns the admin-dashboard runtime statistics as a struct:
 *
 *   { state: "Running", version: "WebStrada v0.1.0",
 *     uptimeSeconds, requestsServed, avgResponseMs,
 *     recentRequests: [ { time, template, method, status, durationMs }, ... ] }
 *
 * The values come from webstrada::stats (per worker process, "since last
 * restart"), tracked by worker::process_request. An optional truthy first
 * argument (excludeAdmin) drops recent requests whose template path starts
 * with /admin — used by the dashboard's "hide admin requests" switch, so the
 * filtering happens server-side.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/server_stats.h>
#include <webstrada/config.h>

#include <cstdint>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf___serverinfo(const cfvariant **args, int argc)
{
    bool excludeAdmin = false;
    int limit = 10;
    int64_t beforeId = 0;
    int64_t sinceId = 0;

    if (argc >= 1 && args && args[0]) {
        excludeAdmin = cfvariant_is_truthy(args[0]);
    }
    if (argc >= 2 && args && args[1]) {
        limit = toInt(args[1]);
        if (limit <= 0) limit = 10;
    }
    if (argc >= 3 && args && args[2]) {
        beforeId = toInt(args[2]);
        if (beforeId < 0) beforeId = 0;
    }
    if (argc >= 4 && args && args[3]) {
        sinceId = toInt(args[3]);
        if (sinceId < 0) sinceId = 0;
    }

    cfvariant root(cfvariant::Struct);

    cfvariant state("Running");
    root.structSet("state", state);
    root.structSet("version", cfvariant("WebStrada v0.1.0"));

    cfvariant let(cfvariant::Boolean);
    let.m_bool = webstrada::config::lineExecutionTrace;
    root.structSet("lineExecutionTrace", let);

    cfvariant har(cfvariant::Boolean);
    har.m_bool = webstrada::stats::hide_admin_requests();
    root.structSet("hideAdminRequests", har);

    cfvariant tsc(cfvariant::Number);
    tsc.m_int = webstrada::stats::trace_session_count();
    root.structSet("traceSessionCount", tsc);

    cfvariant uptime(cfvariant::Long);
    uptime.m_long = webstrada::stats::uptime_seconds();
    root.structSet("uptimeSeconds", uptime);

    cfvariant served(cfvariant::Long);
    served.m_long = webstrada::stats::requests_served();
    root.structSet("requestsServed", served);

    cfvariant avg(cfvariant::Float);
    avg.m_double = webstrada::stats::avg_response_ms();
    root.structSet("avgResponseMs", avg);

    const auto &allRecent = webstrada::stats::recent_requests();
    std::vector<const webstrada::stats::RecentRequest *> filtered;
    filtered.reserve(allRecent.size());
    for (const auto &r : allRecent) {
        if (excludeAdmin && r.templatePath.rfind("/admin", 0) == 0) {
            continue;
        }
        filtered.push_back(&r);
    }

    cfvariant totalFiltered(cfvariant::Long);
    totalFiltered.m_long = static_cast<int64_t>(filtered.size());
    root.structSet("totalRecentRequests", totalFiltered);

    cfvariant recent(cfvariant::Array);
    int total = static_cast<int>(filtered.size());

    if (total > 0) {
        int startIdx = 0;
        int endIdx = total;

        if (beforeId > 0) {
            // Keyset cursor: items with id < beforeId (fetching older records)
            auto it = std::lower_bound(filtered.begin(), filtered.end(), beforeId,
                [](const webstrada::stats::RecentRequest *req, int64_t val) {
                    return req->id < val;
                });
            endIdx = static_cast<int>(std::distance(filtered.begin(), it));
            startIdx = (limit > 0) ? std::max(0, endIdx - limit) : 0;
        } else if (sinceId > 0) {
            // Keyset cursor: items with id > sinceId (fetching newer records)
            auto it = std::upper_bound(filtered.begin(), filtered.end(), sinceId,
                [](int64_t val, const webstrada::stats::RecentRequest *req) {
                    return val < req->id;
                });
            startIdx = static_cast<int>(std::distance(filtered.begin(), it));
            endIdx = (limit > 0) ? std::min(total, startIdx + limit) : total;
        } else {
            // Default: newest `limit` items
            startIdx = (limit > 0) ? std::max(0, total - limit) : 0;
            endIdx = total;
        }

        if (endIdx > startIdx && startIdx >= 0) {
            for (int i = startIdx; i < endIdx; ++i) {
                const auto &r = *filtered[i];
                cfvariant row(cfvariant::Struct);
                cfvariant rid(cfvariant::Long);
                rid.m_long = r.id;
                row.structSet("id", rid);
                cfvariant t(cfvariant::Long);
                t.m_long = r.time;
                row.structSet("time", t);
                row.structSet("template", cfvariant(r.templatePath.c_str()));
                row.structSet("method", cfvariant(r.method.c_str()));
                cfvariant st(cfvariant::Number);
                st.m_int = r.status;
                row.structSet("status", st);
                cfvariant d(cfvariant::Float);
                d.m_double = r.durationMs;
                row.structSet("durationMs", d);
                recent.m_array->push_back(row);
            }
        }
    }
    root.structSet("recentRequests", recent);

    return new cfvariant(root);
}

} // namespace cfml

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

#include <cstdint>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf___serverinfo(const cfvariant **args, int argc)
{
    bool excludeAdmin = false;
    if (argc >= 1 && args && args[0]) {
        excludeAdmin = cfvariant_is_truthy(args[0]);
    }

    cfvariant root(cfvariant::Struct);

    cfvariant state("Running");
    root.structSet("state", state);
    root.structSet("version", cfvariant("WebStrada v0.1.0"));

    cfvariant uptime(cfvariant::Long);
    uptime.m_long = webstrada::stats::uptime_seconds();
    root.structSet("uptimeSeconds", uptime);

    cfvariant served(cfvariant::Long);
    served.m_long = webstrada::stats::requests_served();
    root.structSet("requestsServed", served);

    cfvariant avg(cfvariant::Float);
    avg.m_double = webstrada::stats::avg_response_ms();
    root.structSet("avgResponseMs", avg);

    cfvariant recent(cfvariant::Array);
    for (const auto &r : webstrada::stats::recent_requests()) {
        if (excludeAdmin && r.templatePath.rfind("/admin", 0) == 0) {
            continue;
        }
        cfvariant row(cfvariant::Struct);
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
    root.structSet("recentRequests", recent);

    return new cfvariant(root);
}

} // namespace cfml

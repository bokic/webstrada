#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Server-wide runtime statistics for the admin panel dashboard. Tracked by the
// daemon worker per request (see worker::process_request) and surfaced through
// the __serverInfo() compiler-extension function. Prefork workers each track
// their own counts (the dev server runs a single worker); the values are
// per-process and reset on restart, like ColdFusion's "since last restart".

namespace webstrada {
namespace stats {

struct RecentRequest {
    int64_t id = 0;            // sequential request ID
    int64_t time = 0;          // unix epoch seconds when the request finished
    std::string templatePath;  // REQUEST_URI of the served template
    std::string method;        // GET / POST / ...
    int status = 200;          // response status code
    double durationMs = 0;     // wall-clock time spent handling the request
};

// Records the start of a request (called after the CGI scope is filled).
void request_begin(const std::string &method, const std::string &templatePath);

// Records the end of a request with its response status code.
void request_end(int statusCode, int64_t reqId = 0);

// Seconds since the worker process started.
int64_t uptime_seconds();

// Total number of completed requests.
int64_t requests_served();

// Average wall-clock response time over all completed requests (ms).
double avg_response_ms();

// The most recent requests, oldest first (bounded).
const std::vector<RecentRequest> &recent_requests();

// Clear the recent requests list.
void clear_recent_requests();

// Trace session request tracking (auto-stop after 100 requests)
int increment_trace_session_count();
void reset_trace_session_count();
int trace_session_count();

// In-memory setting to exclude /admin requests from execution tracing and dashboard stats (default true)
bool hide_admin_requests();
void set_hide_admin_requests(bool hide);

} // namespace stats
} // namespace webstrada

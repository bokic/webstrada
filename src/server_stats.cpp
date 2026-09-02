// Server-wide runtime statistics (see webstrada/server_stats.h). Each daemon
// worker tracks its own request counters in-process; the values feed the admin
// dashboard through __serverInfo().

#include <webstrada/server_stats.h>

#include <chrono>
#include <ctime>
#include <mutex>

namespace webstrada {
namespace stats {

namespace {

constexpr size_t kMaxRecent = 1000;

int64_t g_startTime = static_cast<int64_t>(std::time(nullptr));
int64_t g_requests = 0;
double g_totalMs = 0;
std::vector<RecentRequest> g_recent;
int g_traceSessionCount = 0;
std::mutex g_mutex;

// The request currently being handled (the worker processes one request at a
// time, so a plain thread_local is enough).
thread_local double t_reqStartMs = 0;
thread_local std::string t_method;
thread_local std::string t_templatePath;

double nowMs()
{
    using namespace std::chrono;
    return duration_cast<duration<double, std::milli>>(
               steady_clock::now().time_since_epoch())
        .count();
}

} // namespace

void request_begin(const std::string &method, const std::string &templatePath)
{
    t_method = method;
    t_templatePath = templatePath;
    t_reqStartMs = nowMs();
}

void request_end(int statusCode, int64_t reqId)
{
    double durationMs = nowMs() - t_reqStartMs;
    std::lock_guard<std::mutex> lock(g_mutex);
    ++g_requests;
    g_totalMs += durationMs;
    RecentRequest rr;
    rr.id = (reqId > 0) ? reqId : g_requests;
    rr.time = static_cast<int64_t>(std::time(nullptr));
    rr.templatePath = t_templatePath;
    rr.method = t_method;
    rr.status = statusCode;
    rr.durationMs = durationMs;
    g_recent.push_back(std::move(rr));
    if (g_recent.size() > kMaxRecent) {
        g_recent.erase(g_recent.begin());
    }
}

int64_t uptime_seconds()
{
    return static_cast<int64_t>(std::time(nullptr)) - g_startTime;
}

int64_t requests_served()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_requests;
}

double avg_response_ms()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_requests > 0 ? g_totalMs / static_cast<double>(g_requests) : 0.0;
}

const std::vector<RecentRequest> &recent_requests()
{
    return g_recent;
}

void clear_recent_requests()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_recent.clear();
}

int increment_trace_session_count()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return ++g_traceSessionCount;
}

void reset_trace_session_count()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_traceSessionCount = 0;
}

int trace_session_count()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_traceSessionCount;
}

static bool g_hideAdminRequests = true;

bool hide_admin_requests()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_hideAdminRequests;
}

void set_hide_admin_requests(bool hide)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_hideAdminRequests = hide;
}

bool is_admin_request_path(const std::string &path)
{
    return path.rfind("/webstrada", 0) == 0;
}

} // namespace stats
} // namespace webstrada

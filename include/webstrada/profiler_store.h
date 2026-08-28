#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct sqlite3;

namespace webstrada {

struct TraceStep {
    int seq = 0;
    std::string type; // "ENTRY", "LINE", "EXIT", "DB_QUERY_START", etc.
    std::string path;
    std::string function;
    int line = 0;
    std::string stackTrace; // '|' separated call stack
    double durationMs = 0.0;
    double timestampMs = 0.0;
};

struct RequestTraceSummary {
    double timestamp = 0.0;
    std::string method;
    std::string url;
    int status = 200;
    double durationMs = 0.0;
    double onRequestStartMs = 0.0;
    double pageExecutionMs = 0.0;
    double onRequestEndMs = 0.0;
    int dbQueriesCount = 0;
    double dbQueriesMs = 0.0;
    int customTagsCount = 0;
    double customTagsMs = 0.0;
    int cfcMethodsCount = 0;
    double cfcMethodsMs = 0.0;
    std::vector<TraceStep> steps;
};

class ProfilerStore
{
public:
    ProfilerStore() = default;
    ~ProfilerStore();

    ProfilerStore(const ProfilerStore &) = delete;
    ProfilerStore &operator=(const ProfilerStore &) = delete;

    bool open(const std::string &dbPath);
    void close();

    bool isOpen() const { return m_db != nullptr; }
    const std::string &lastError() const { return m_lastError; }

    int64_t recordRequest(const RequestTraceSummary &summary);
    bool getRequestSteps(int64_t requestId, std::vector<TraceStep> &steps);

private:
    bool exec(const char *sql);

    sqlite3 *m_db = nullptr;
    std::string m_lastError;
};

ProfilerStore &profiler_store();
void open_profiler_store();

} // namespace webstrada

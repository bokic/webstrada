#include <webstrada/profiler_store.h>

#include <sqlite3.h>
#include <cstring>
#include <cstdio>
#include <unistd.h>

namespace webstrada {

ProfilerStore::~ProfilerStore()
{
    close();
}

bool ProfilerStore::exec(const char *sql)
{
    char *err = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        m_lastError = err ? err : "sqlite3_exec failed";
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool ProfilerStore::open(const std::string &dbPath)
{
    if (m_db) close();

    if (sqlite3_open(dbPath.c_str(), &m_db) != SQLITE_OK) {
        m_lastError = m_db ? sqlite3_errmsg(m_db) : "sqlite3_open failed";
        if (m_db) sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    if (!exec("PRAGMA journal_mode=WAL;")) return false;
    if (!exec("PRAGMA synchronous=NORMAL;")) return false;
    if (!exec("PRAGMA temp_store=MEMORY;")) return false;
    if (!exec("PRAGMA busy_timeout=5000;")) return false;

    static const char *kSchema =
        "CREATE TABLE IF NOT EXISTS requests ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " timestamp REAL,"
        " method TEXT,"
        " url TEXT,"
        " status INTEGER,"
        " duration_ms REAL,"
        " on_request_start_ms REAL,"
        " page_execution_ms REAL,"
        " on_request_end_ms REAL,"
        " db_queries_count INTEGER,"
        " db_queries_ms REAL,"
        " custom_tags_count INTEGER,"
        " custom_tags_ms REAL,"
        " cfc_methods_count INTEGER,"
        " cfc_methods_ms REAL,"
        " created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS request_traces ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " request_id INTEGER,"
        " seq INTEGER,"
        " event_type TEXT,"
        " path TEXT,"
        " function_name TEXT,"
        " line_number INTEGER,"
        " stack_trace TEXT,"
        " delta_ms REAL,"
        " elapsed_ms REAL,"
        " FOREIGN KEY(request_id) REFERENCES requests(id) ON DELETE CASCADE"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_traces_req ON request_traces(request_id);"
        "CREATE INDEX IF NOT EXISTS idx_traces_req_seq ON request_traces(request_id, seq);"
        "CREATE INDEX IF NOT EXISTS idx_requests_timestamp ON requests(timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_requests_url ON requests(url);";

    if (!exec(kSchema)) return false;

    exec("ALTER TABLE request_traces ADD COLUMN stack_trace TEXT;");
    exec("CREATE INDEX IF NOT EXISTS idx_traces_req_seq ON request_traces(request_id, seq);");
    exec("CREATE INDEX IF NOT EXISTS idx_requests_timestamp ON requests(timestamp);");
    exec("CREATE INDEX IF NOT EXISTS idx_requests_url ON requests(url);");

    return true;
}

void ProfilerStore::close()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

int64_t ProfilerStore::recordRequest(const RequestTraceSummary &summary)
{
    if (!m_db) return 0;

    if (!exec("BEGIN IMMEDIATE;")) return 0;

    static const char *kInsertReq =
        "INSERT INTO requests ("
        " timestamp, method, url, status, duration_ms,"
        " on_request_start_ms, page_execution_ms, on_request_end_ms,"
        " db_queries_count, db_queries_ms,"
        " custom_tags_count, custom_tags_ms,"
        " cfc_methods_count, cfc_methods_ms"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmtReq = nullptr;
    if (sqlite3_prepare_v2(m_db, kInsertReq, -1, &stmtReq, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        exec("ROLLBACK;");
        return 0;
    }

    sqlite3_bind_double(stmtReq, 1, summary.timestamp);
    sqlite3_bind_text(stmtReq, 2, summary.method.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtReq, 3, summary.url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmtReq, 4, summary.status);
    sqlite3_bind_double(stmtReq, 5, summary.durationMs);
    sqlite3_bind_double(stmtReq, 6, summary.onRequestStartMs);
    sqlite3_bind_double(stmtReq, 7, summary.pageExecutionMs);
    sqlite3_bind_double(stmtReq, 8, summary.onRequestEndMs);
    sqlite3_bind_int(stmtReq, 9, summary.dbQueriesCount);
    sqlite3_bind_double(stmtReq, 10, summary.dbQueriesMs);
    sqlite3_bind_int(stmtReq, 11, summary.customTagsCount);
    sqlite3_bind_double(stmtReq, 12, summary.customTagsMs);
    sqlite3_bind_int(stmtReq, 13, summary.cfcMethodsCount);
    sqlite3_bind_double(stmtReq, 14, summary.cfcMethodsMs);

    if (sqlite3_step(stmtReq) != SQLITE_DONE) {
        m_lastError = sqlite3_errmsg(m_db);
        sqlite3_finalize(stmtReq);
        exec("ROLLBACK;");
        return 0;
    }
    sqlite3_finalize(stmtReq);

    int64_t reqId = sqlite3_last_insert_rowid(m_db);

    if (!summary.steps.empty()) {
        static const char *kInsertTrace =
            "INSERT INTO request_traces ("
            " request_id, seq, event_type, path, function_name, line_number, stack_trace, delta_ms, elapsed_ms"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

        sqlite3_stmt *stmtTrace = nullptr;
        if (sqlite3_prepare_v2(m_db, kInsertTrace, -1, &stmtTrace, nullptr) != SQLITE_OK) {
            m_lastError = sqlite3_errmsg(m_db);
            exec("ROLLBACK;");
            return 0;
        }

        int seq = 0;
        for (const auto &step : summary.steps) {
            sqlite3_bind_int64(stmtTrace, 1, reqId);
            sqlite3_bind_int(stmtTrace, 2, ++seq);
            sqlite3_bind_text(stmtTrace, 3, step.type.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmtTrace, 4, step.path.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmtTrace, 5, step.function.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmtTrace, 6, step.line);
            sqlite3_bind_text(stmtTrace, 7, step.stackTrace.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmtTrace, 8, step.durationMs);
            sqlite3_bind_double(stmtTrace, 9, step.timestampMs);

            sqlite3_step(stmtTrace);
            sqlite3_reset(stmtTrace);
        }
        sqlite3_finalize(stmtTrace);
    }

    if (exec("COMMIT;")) {
        return reqId;
    }
    return 0;
}

bool ProfilerStore::getRequestSteps(int64_t requestId, std::vector<TraceStep> &steps)
{
    if (!m_db) return false;

    static const char *kQuery =
        "SELECT seq, event_type, path, function_name, line_number, stack_trace, delta_ms, elapsed_ms "
        "FROM request_traces WHERE request_id = ? ORDER BY seq ASC;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, kQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }

    sqlite3_bind_int64(stmt, 1, requestId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TraceStep step;
        step.seq = sqlite3_column_int(stmt, 0);
        const unsigned char *type = sqlite3_column_text(stmt, 1);
        const unsigned char *path = sqlite3_column_text(stmt, 2);
        const unsigned char *func = sqlite3_column_text(stmt, 3);
        int line = sqlite3_column_int(stmt, 4);
        const unsigned char *st = sqlite3_column_text(stmt, 5);
        double delta = sqlite3_column_double(stmt, 6);
        double elapsed = sqlite3_column_double(stmt, 7);

        if (type) step.type = reinterpret_cast<const char *>(type);
        if (path) step.path = reinterpret_cast<const char *>(path);
        if (func) step.function = reinterpret_cast<const char *>(func);
        step.line = line;
        if (st) step.stackTrace = reinterpret_cast<const char *>(st);
        step.durationMs = delta;
        step.timestampMs = elapsed;

        steps.push_back(std::move(step));
    }
    sqlite3_finalize(stmt);
    return true;
}

namespace {
ProfilerStore g_profilerStore;
}

ProfilerStore &profiler_store()
{
    return g_profilerStore;
}

void open_profiler_store()
{
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    std::string dbPath;
    if (n > 0) {
        exe[n] = '\0';
        std::string path(exe);
        size_t slash = path.find_last_of('/');
        dbPath = (slash != std::string::npos)
            ? path.substr(0, slash + 1) + "WebStrada-profiler.sqlite"
            : "WebStrada-profiler.sqlite";
    } else {
        dbPath = "WebStrada-profiler.sqlite";
    }
    if (!g_profilerStore.isOpen()) {
        g_profilerStore.open(dbPath);
    }
}

} // namespace webstrada

#include <webstrada/profiler_store.h>

#include <sqlite3.h>
#include <cstring>
#include <cstdio>

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

bool ProfilerStore::recordRequest(const RequestTraceSummary &summary)
{
    if (!m_db) return false;

    if (!exec("BEGIN IMMEDIATE;")) return false;

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
        return false;
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
        return false;
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
            return false;
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
            sqlite3_bind_double(stmtTrace, 8, step.deltaMs);
            sqlite3_bind_double(stmtTrace, 9, step.elapsedMs);

            sqlite3_step(stmtTrace);
            sqlite3_reset(stmtTrace);
        }
        sqlite3_finalize(stmtTrace);
    }

    return exec("COMMIT;");
}

} // namespace webstrada

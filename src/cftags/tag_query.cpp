/**
 * @file tag_query.cpp
 * @brief <cfquery> runtime (cf_query_begin / cf_query_end).
 *
 * The SQL execution itself lives in the abstract database layer
 * (include/webstrada/db.h): cf_run_query opens a connection through the driver
 * registry (SQLite backend) and executes the evaluated SQL text. This file
 * keeps the CFML-observable parts: attribute extraction, the result struct
 * (SQL/RECORDCOUNT/CACHED/COLUMNLIST/GENERATEDKEY/EXECUTIONTIME) and the
 * `name` / `result` variable assignment.
 */

#include "common.h"
#include "../core/core_internal.h"

#include <webstrada/cf8.h>
#include <webstrada/cache_store.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/db.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace cfml {

// Owns the DB connection cf_run_query opens when no transaction does; closes it
// on every exit path, including exceptions thrown after the connection is
// opened (SQL errors, result building, cache stores, cfvariant_assign).
struct ConnectionGuard {
    db::DBConnection *conn;
    bool owned;
    ConnectionGuard(db::DBConnection *c, bool owned) : conn(c), owned(owned) {}
    ~ConnectionGuard() {
        if (owned && conn) delete conn;
    }
};

string *cf_query_begin()
{
    return silent_buf_push();
}

// Query of Queries (<cfquery dbtype="query">): the SQL runs against the query
// objects named in its FROM/JOIN clauses (resolved from the CFML scopes), which
// the db layer materializes on demand into an in-memory SQLite database.
cfvariant *cf_run_query_of_queries(const std::string &sql, const cfvariant *attrs,
                                   const std::string &qname, const std::string &resultName,
                                   long long maxrows,
                                   void *cgi, void *server, void *cookie, void *application,
                                   void *session, void *url, void *form, void *variables)
{
    (void)attrs;
    // Resolve a QoQ table name to a query object. The db layer calls this for
    // each distinct missing table, so only the referenced queries are
    // materialized. An undefined name or a non-query value throws CF's QoQ
    // error (verified against CF 2025/2021 on the RDS host: type "Database",
    // message "Error Executing Database Query.", detail "Query Of Queries
    // runtime error.").
    db::QoQResolver resolver = [&](const std::string &name) -> const cfvariant * {
        auto *v = lookupVarWritable(name.c_str(), cgi, server, cookie, application,
                                    session, url, form, variables);
        if (!v || v->m_type != cfvariant::Query || !v->m_query) {
            throw webstrada::exception("Database", "Error Executing Database Query.",
                "Query Of Queries runtime error.");
        }
        return v;
    };

    if (webstrada::config::enableQueryLogging) {
        printf("[cfquery] dbtype=query name=%s\n%s\n", qname.c_str(), sql.c_str());
        fflush(stdout);
    }

    auto execStart = std::chrono::steady_clock::now();
    db::DBResult result = db::runQueryOfQueries(sql, maxrows, resolver);
    auto execEnd = std::chrono::steady_clock::now();
    long long execTime = std::chrono::duration_cast<std::chrono::milliseconds>(execEnd - execStart).count();

    if (webstrada::config::enableQueryLogging) {
        printf("[cfquery]   -> %lld rows in %lldms\n",
               static_cast<long long>(result.rowCount), execTime);
        fflush(stdout);
    }

    cfvariant queryVal(cfvariant::Query);
    QueryData *qd = queryVal.m_query;
    for (auto &c : result.columns) {
        QueryColumn col;
        col.name = c.name.c_str();
        col.type = c.type.c_str();
        col.values = std::move(c.values);
        qd->columns.push_back(std::move(col));
    }
    qd->m_rowCount = static_cast<int>(result.rowCount);

    if (!qname.empty() && !qd->columns.empty()) {
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         qname.c_str(), &queryVal);
    }

    if (!resultName.empty()) {
        cfvariant resultStruct(cfvariant::Struct);
        resultStruct.set("SQL") = cfvariant(sql.c_str());
        resultStruct.set("RECORDCOUNT") = cfvariant(static_cast<int>(result.rowCount));
        resultStruct.set("CACHED") = cfvariant("false");
        if (!qd->columns.empty()) {
            resultStruct.set("COLUMNLIST") = cfvariant(queryColumnList(&queryVal));
        }
        if (result.hasGeneratedKey) {
            cfvariant gv(cfvariant::Long);
            gv.m_long = result.generatedKey;
            resultStruct.set("GENERATEDKEY") = gv;
        }
        resultStruct.set("EXECUTIONTIME") = [&] {
            cfvariant v(cfvariant::Long);
            v.m_long = execTime;
            return v;
        }();
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         resultName.c_str(), &resultStruct);
    }

    auto *ret = new cfvariant(queryVal);
    cf_register_temp(ret);
    return ret;
}

// Shared SQL execution core used by <cfquery> (cf_query_end) and the
// queryExecute() built-in. `sql` is the final SQL text (already evaluated /
// parameter-substituted); `attrs` is the attribute/options struct. Returns the
// result query object (also stored under `name` / `result` when given).
cfvariant *cf_run_query(const std::string &sqlIn, const cfvariant *attrs,
                        void *cgi, void *server, void *cookie, void *application,
                        void *session, void *url, void *form, void *variables)
{
    std::string sql = sqlIn;

    // Extract attributes
    auto attr = [&](const char *key) -> const cfvariant * {
        if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
        string k(key);
        auto it = attrs->m_struct->find(k);
        return it == attrs->m_struct->end() ? nullptr : &it->second;
    };

    std::string qname = attr("name") ? safe_to_std_string(*attr("name")) : "";
    std::string dsn = attr("datasource") ? safe_to_std_string(*attr("datasource")) : "";
    std::string dbtype = attr("dbtype") ? safe_to_std_string(*attr("dbtype")) : "";
    std::string resultName = attr("result") ? safe_to_std_string(*attr("result")) : "";
    std::string username = attr("username") ? safe_to_std_string(*attr("username")) : "";
    std::string password = attr("password") ? safe_to_std_string(*attr("password")) : "";
    long long maxrows = attr("maxrows") ? cfvariant_to_long(attr("maxrows")) : -1;
    long long timeout = attr("timeout") ? cfvariant_to_long(attr("timeout")) : -1;

    (void)username;
    (void)password;

    if (dbtype.empty() && dsn.empty()) {
        throw webstrada::exception("You have specified no data source and no dbtype. Choose one or the other.");
    }
    if (!dbtype.empty()) {
        std::string dt = dbtype;
        for (auto &c : dt) c = static_cast<char>(toupper(c));
        if (dt == "QUERY") {
            return cf_run_query_of_queries(sql, attrs, qname, resultName, maxrows,
                                           cgi, server, cookie, application, session, url, form, variables);
        }
        if (dt == "HQL") {
            throw webstrada::exception("cfquery", ("dbtype='" + dbtype + "' (HQL) is not implemented yet.").c_str());
        }
        throw webstrada::exception("cfquery", ("The dbtype '" + dbtype + "' is not supported.").c_str());
    }

    if (sql.empty()) {
        throw webstrada::exception("Database", "Error Executing Database Query.",
            "The SQL statement is empty.");
    }

    // ---- Query cache (cachedwithin / cachedafter / cacheid / cacheregion) ----
    // Mirrors CF's QueryTag.setupCachedQuery: when any cache attribute is
    // present, the result is keyed (cacheid, else a hash of sql+datasource)
    // in the CacheStore's QUERY region (or cacheregion). cachedwithin (a
    // timespan in days) bounds the freshness; cachedafter caches only when the
    // entry is newer than the given date; a cacheid alone caches eternally.
    std::string cacheId = attr("cacheid") ? safe_to_std_string(*attr("cacheid")) : "";
    std::string cacheRegion = attr("cacheregion") ? safe_to_std_string(*attr("cacheregion")) : "";
    bool hasCachedWithin = attr("cachedwithin") != nullptr;
    bool hasCachedAfter = attr("cachedafter") != nullptr;
    int64_t cachedWithinMs = 0;
    if (hasCachedWithin) {
        double days = getDoubleValue(*attr("cachedwithin"));
        cachedWithinMs = (int64_t)(days * 86400.0 * 1000.0);
    }
    int64_t cachedAfterMs = 0;
    if (hasCachedAfter) {
        cfvariant dt = *attr("cachedafter");
        // A DateTime converts to days since 1899-12-30; convert to epoch ms.
        if (dt.m_type == cfvariant::DateTime) {
            double days = dt.m_double;
            time_t epoch = (time_t)((days - 25569.0) * 86400.0);
            cachedAfterMs = (int64_t)epoch * 1000;
        }
    }
    bool useQueryCache = hasCachedWithin || hasCachedAfter || !cacheId.empty() || !cacheRegion.empty();
    if (useQueryCache) {
        webstrada::CacheStore &cstore = webstrada::cache_store();
        if (cstore.isOpen()) {
            std::string regionName = cacheRegion.empty() ? "QUERY" : cacheRegion;
            const cfvariant *paramsArr = attr("params");
            cfvariant cacheIdVal(cacheId.c_str());
            std::string key = cf_query_cache_key(sql, dsn, paramsArr, cacheId.empty() ? nullptr : &cacheIdVal);
            int64_t now = static_cast<int64_t>(::time(nullptr)) * 1000;
            std::string cachedJson;
            bool hit = cstore.get(regionName, key, now, cachedJson, /*quiet=*/false);
            if (hit) {
                cfvariant jsonVal(cachedJson.c_str());
                cfvariant *cachedQuery = cf_query_cache_deserialize(&jsonVal);
                // cachedafter: reuse only when the entry is newer than the date.
                if (hasCachedAfter) {
                    webstrada::CacheStore::EntryMeta meta = cstore.metadata(regionName, key, now);
                    if (meta.found && meta.createdMs < cachedAfterMs) {
                        delete cachedQuery;
                        cachedQuery = nullptr;
                        hit = false;
                    }
                }
                if (hit) {
                    cfvariant queryVal = *cachedQuery;
                    delete cachedQuery;
                    if (!qname.empty() && queryVal.m_query && !queryVal.m_query->columns.empty()) {
                        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                                         qname.c_str(), &queryVal);
                    }
                    if (!resultName.empty()) {
                        cfvariant resultStruct(cfvariant::Struct);
                        resultStruct.set("SQL") = cfvariant(sql.c_str());
                        resultStruct.set("RECORDCOUNT") = cfvariant(queryVal.m_query ? queryVal.m_query->rowCount() : 0);
                        resultStruct.set("CACHED") = cfvariant("true");
                        if (queryVal.m_query && !queryVal.m_query->columns.empty()) {
                            resultStruct.set("COLUMNLIST") = cfvariant(queryColumnList(&queryVal));
                        }
                        cfvariant ev(cfvariant::Long);
                        ev.m_long = 0;
                        resultStruct.set("EXECUTIONTIME") = ev;
                        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                                         resultName.c_str(), &resultStruct);
                    }
                    auto *ret = new cfvariant(queryVal);
                    cf_register_temp(ret);
                    return ret;
                }
            }
        }
    }

    TxFrame *tx = transaction_get_active(dsn);
    bool connOwnedByTx = tx && tx->conn;

    db::DBConnection *conn = nullptr;
    if (connOwnedByTx) {
        conn = tx->conn;
    } else {
        long long timeoutMs = timeout >= 0 ? timeout * 1000 : 0;
        conn = db::getConnection(dsn, timeoutMs);
        if (tx) {
            tx->conn = conn;
            tx->inTransaction = true;
            conn->begin();
            connOwnedByTx = true;
        }
    }

    // Dev-server query log: each executed statement is printed to stdout before
    // it runs so a request's SQL (including a statement that then fails) is
    // visible in the terminal; http-dev.py tees the daemon's stdout to the
    // console. The CLI and unit tests disable this via config::enableQueryLogging
    // because their stdout is compared byte-for-byte against CF.
    if (webstrada::config::enableQueryLogging) {
        printf("[cfquery] dsn=%s name=%s\n%s\n", dsn.c_str(), qname.c_str(), sql.c_str());
        fflush(stdout);
    }

    std::string dsnUp = dsn;
    for (auto &c : dsnUp) c = static_cast<char>(toupper((unsigned char)c));

    bool isFirstQueryForDsn = (g_requestQueriedDsns.find(dsnUp) == g_requestQueriedDsns.end());
    bool canRetryOnDisconnect = (!connOwnedByTx && isFirstQueryForDsn && g_requestRetriedDsns.find(dsnUp) == g_requestRetriedDsns.end());

    cfml::trace_record_event("DB_QUERY_START", dsn.c_str(), qname.c_str(), 0);
    auto execStart = std::chrono::steady_clock::now();
    db::DBResult result;
    try {
        result = conn->execute(sql, maxrows);
    } catch (const webstrada::exception &ex) {
        std::string detail = ex.m_detail.constData() ? ex.m_detail.constData() : "";
        if (canRetryOnDisconnect && db::isConnectionLost(detail)) {
            g_requestRetriedDsns.insert(dsnUp);
            long long timeoutMs = timeout >= 0 ? timeout * 1000 : 0;
            conn = db::reopenConnection(dsn, timeoutMs);
            result = conn->execute(sql, maxrows);
        } else {
            cfml::trace_record_event("DB_QUERY_END", dsn.c_str(), qname.c_str(), 0);
            g_requestQueriedDsns.insert(dsnUp);
            throw;
        }
    }
    cfml::trace_record_event("DB_QUERY_END", dsn.c_str(), qname.c_str(), 0);
    g_requestQueriedDsns.insert(dsnUp);
    auto execEnd = std::chrono::steady_clock::now();
    double queryMs = std::chrono::duration<double, std::milli>(execEnd - execStart).count();
    g_reqProfiler.queryCount++;
    g_reqProfiler.queryTime += queryMs;
    long long execTime = std::chrono::duration_cast<std::chrono::milliseconds>(execEnd - execStart).count();

    long long rowCount = result.rowCount;
    bool hasGeneratedKey = result.hasGeneratedKey;
    long long generatedKey = result.generatedKey;

    if (webstrada::config::enableQueryLogging) {
        printf("[cfquery]   -> %lld rows in %lldms\n",
               static_cast<long long>(rowCount), execTime);
        fflush(stdout);
    }

    cfvariant queryVal(cfvariant::Query);
    QueryData *qd = queryVal.m_query;
    for (auto &c : result.columns) {
        QueryColumn col;
        col.name = c.name.c_str();
        col.type = c.type.c_str();
        col.values = std::move(c.values);
        qd->columns.push_back(std::move(col));
    }
    qd->m_rowCount = static_cast<int>(rowCount);

    // Store the freshly-executed result in the query cache when a cache
    // attribute was present (and we did not serve a cache hit above).
    if (useQueryCache) {
        webstrada::CacheStore &cstore = webstrada::cache_store();
        if (cstore.isOpen()) {
            std::string regionName = cacheRegion.empty() ? "QUERY" : cacheRegion;
            cfvariant cacheIdVal(cacheId.c_str());
            const cfvariant *paramsArr = attr("params");
            std::string key = cf_query_cache_key(sql, dsn, paramsArr, cacheId.empty() ? nullptr : &cacheIdVal);
            cfvariant *blob = cf_query_cache_serialize(&queryVal);
            webstrada::string bstr = blob->toString();
            std::string blobJson = bstr.constData() ? bstr.constData() : "";
            delete blob;
            int64_t now = static_cast<int64_t>(::time(nullptr)) * 1000;
            // cachedwithin bounds the TTL; cachedafter / cacheid alone keep the
            // entry until it naturally expires (eternal when no interval given).
            int64_t ttl = hasCachedWithin ? cachedWithinMs : 0;
            int64_t idle = 0;
            cstore.put(regionName, key, blobJson, ttl, idle, now);
        }
    }

    if (!qname.empty() && !qd->columns.empty()) {
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         qname.c_str(), &queryVal);
    }

    if (!resultName.empty()) {
        cfvariant resultStruct(cfvariant::Struct);
        resultStruct.set("SQL") = cfvariant(sql.c_str());
        resultStruct.set("RECORDCOUNT") = cfvariant(static_cast<int>(rowCount));
        resultStruct.set("CACHED") = cfvariant("false");
        if (!qd->columns.empty()) {
            resultStruct.set("COLUMNLIST") = cfvariant(queryColumnList(&queryVal));
        }
        if (hasGeneratedKey) {
            cfvariant gv(cfvariant::Long);
            gv.m_long = generatedKey;
            resultStruct.set("GENERATEDKEY") = gv;
        }
        resultStruct.set("EXECUTIONTIME") = [&] {
            cfvariant v(cfvariant::Long);
            v.m_long = execTime;
            return v;
        }();
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         resultName.c_str(), &resultStruct);
    }

    auto *ret = new cfvariant(queryVal);
    cf_register_temp(ret);
    return ret;
}

cfvariant *cf_query_end(string *sqlCapture, const cfvariant *attrs,
                        void *cgi, void *server, void *cookie, void *application,
                        void *session, void *url, void *form, void *variables)
{
    // Pop the capture buffer and take its content (the evaluated SQL text).
    std::string sql;
    if (sqlCapture) {
        const char *d = sqlCapture->constData();
        if (d) sql.assign(d, sqlCapture->length());
    }
    silent_buf_pop();
    size_t b = 0, e = sql.size();
    while (b < e && static_cast<unsigned char>(sql[b]) <= 0x20) b++;
    while (e > b && static_cast<unsigned char>(sql[e - 1]) <= 0x20) e--;
    std::string trimmed = sql.substr(b, e - b);
    sql = std::move(trimmed);

    return cf_run_query(sql, attrs, cgi, server, cookie, application, session, url, form, variables);
}

} // namespace cfml

/**
 * @file tag_storedproc.cpp
 * @brief <cfstoredproc> / <cfprocparam> / <cfprocresult> runtime.
 *
 * cf_storedproc_begin pushes a per-thread call context holding the accumulated
 * parameters and result-set bindings; each compiled <cfprocparam> /
 * <cfprocresult> appends to it via cf_proc_param / cf_proc_result;
 * cf_storedproc_end pops the context, executes the procedure through the
 * abstract DB layer (CALL on MySQL/PostgreSQL) and assigns the result sets,
 * the out/inout parameter values, and the statusCode / executionTime
 * variables like CF. The stack is reset per request (scope_begin) so an
 * exception unwinding past cf_storedproc_end cannot leak a context.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/db.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace cfml {

namespace {

// Read an attribute from the evaluated attribute struct (case-insensitive).
const cfvariant *attrOf(const cfvariant *attrs, const char *key)
{
    if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
    string k(key);
    auto it = attrs->m_struct->find(k);
    return it == attrs->m_struct->end() ? nullptr : &it->second;
}

// Normalize a CFSQLTYPE name (uppercase, optional CF_SQL_ prefix stripped).
std::string normalizeSqlType(const std::string &t)
{
    std::string up;
    for (char c : t) up += static_cast<char>(toupper((unsigned char)c));
    if (up.rfind("CF_SQL_", 0) == 0) up = up.substr(7);
    return up;
}

// Build a Query cfvariant from a db result (columns + rows).
cfvariant queryFromDbResult(const db::DBResult &r)
{
    cfvariant q(cfvariant::Query);
    QueryData *qd = q.m_query;
    for (auto &c : r.columns) {
        QueryColumn col;
        col.name = c.name.c_str();
        col.type = c.type.c_str();
        col.values = c.values;
        qd->columns.push_back(std::move(col));
    }
    qd->m_rowCount = static_cast<int>(r.rowCount);
    return q;
}

// Limit a db result to `maxrows` rows (no-op when maxrows < 0).
db::DBResult limitRows(const db::DBResult &r, long long maxrows)
{
    if (maxrows < 0 || r.rowCount <= maxrows) return r;
    db::DBResult out = r;
    for (auto &col : out.columns) col.values.resize(static_cast<size_t>(maxrows));
    out.rowCount = maxrows;
    return out;
}

std::string trimString(std::string s)
{
    size_t b = 0, e = s.size();
    while (b < e && static_cast<unsigned char>(s[b]) <= 0x20) b++;
    while (e > b && static_cast<unsigned char>(s[e - 1]) <= 0x20) e--;
    return s.substr(b, e - b);
}

} // namespace

// Push a fresh call context and a discard buffer for the tag body (the body's
// output is suppressed; only the cfprocparam/cfprocresult tags matter).
string *cf_storedproc_begin()
{
    g_spCtxs.push_back(new StoredProcCtx());
    return silent_buf_push();
}

// <cfprocparam> runtime: append a parameter to the enclosing <cfstoredproc>.
void cf_proc_param(const cfvariant *type, const cfvariant *variable, const cfvariant *value,
                   const cfvariant *cfsqltype, const cfvariant *maxlength,
                   const cfvariant *scale, const cfvariant *isNull,
                   const cfvariant *dbvarname)
{
    if (g_spCtxs.empty()) {
        throw webstrada::exception("cfprocparam is only valid inside a cfstoredproc tag.");
    }
    StoredProcParam p;
    if (type) p.type = safe_to_std_string(*type);
    for (auto &c : p.type) c = static_cast<char>(tolower((unsigned char)c));
    if (p.type != "out" && p.type != "inout" && p.type != "in") p.type = "in";
    if (variable) p.variable = safe_to_std_string(*variable);
    if (cfsqltype) p.cfsqltype = safe_to_std_string(*cfsqltype);
    p.cfsqltype = normalizeSqlType(p.cfsqltype);
    if (p.cfsqltype.empty()) p.cfsqltype = "CHAR";
    if (dbvarname) p.dbVarName = safe_to_std_string(*dbvarname);
    if (maxlength) p.maxlength = (int)cfvariant_to_long(maxlength);
    if (scale) p.scale = (int)cfvariant_to_long(scale);
    if (isNull) p.isNull = cfvariant_is_truthy(isNull);

    bool isIn = (p.type == "in" || p.type == "inout");
    if (value) p.value = safe_to_std_string(*value);
    // An IN/INOUT parameter without a value (and not null) is a CF validation
    // error ("The value attribute is required for IN/INOUT parameters").
    if (isIn && !p.isNull && value == nullptr) {
        throw webstrada::exception("cfprocparam", "The value attribute is required for IN/INOUT parameters.");
    }
    if (p.isNull) p.value.clear();

    g_spCtxs.back()->params.push_back(std::move(p));
}

// <cfprocresult> runtime: append a result-set binding to the enclosing
// <cfstoredproc>.
void cf_proc_result(const cfvariant *name, const cfvariant *resultset, const cfvariant *maxrows)
{
    if (g_spCtxs.empty()) {
        throw webstrada::exception("cfprocresult is only valid inside a cfstoredproc tag.");
    }
    StoredProcResultBinding b;
    if (name) b.name = safe_to_std_string(*name);
    if (resultset) b.resultset = (int)cfvariant_to_long(resultset);
    if (b.resultset < 1) b.resultset = 1;
    if (maxrows) b.maxrows = cfvariant_to_long(maxrows);
    g_spCtxs.back()->results.push_back(std::move(b));
}

// </cfstoredproc> runtime: pop the context, execute the procedure and assign
// the result sets, out/inout values and the statusCode / executionTime
// variables.
void cf_storedproc_end(const cfvariant *attrs,
                       void *cgi, void *server, void *cookie, void *application,
                       void *session, void *url, void *form, void *variables)
{
    if (g_spCtxs.empty()) {
        throw webstrada::exception("cfstoredproc", "Missing <cfstoredproc> start tag");
    }
    StoredProcCtx ctx = std::move(*g_spCtxs.back());
    delete g_spCtxs.back();
    g_spCtxs.pop_back();
    // Pop the body's discard buffer.
    silent_buf_pop();

    std::string procedure = attrOf(attrs, "procedure") ? safe_to_std_string(*attrOf(attrs, "procedure")) : "";
    std::string dsn = attrOf(attrs, "datasource") ? safe_to_std_string(*attrOf(attrs, "datasource")) : "";
    std::string resultName = attrOf(attrs, "result") ? safe_to_std_string(*attrOf(attrs, "result")) : "";
    bool returncode = attrOf(attrs, "returncode") ? cfvariant_is_truthy(attrOf(attrs, "returncode")) : false;

    if (procedure.empty()) {
        throw webstrada::exception("Attribute validation error for tag CFSTOREDPROC.",
            "It requires the attribute(s): PROCEDURE.");
    }
    if (dsn.empty()) {
        throw webstrada::exception("Attribute validation error for tag CFSTOREDPROC.",
            "It requires the attribute(s): DATASOURCE.");
    }

    // Build the DB-layer parameter list.
    std::vector<db::DBStoredProcParam> dbParams;
    dbParams.reserve(ctx.params.size());
    for (auto &p : ctx.params) {
        db::DBStoredProcParam dp;
        dp.type = p.type;
        dp.name = p.dbVarName;
        dp.cfsqltype = p.cfsqltype;
        dp.value = p.value;
        dp.isNull = p.isNull;
        dbParams.push_back(std::move(dp));
    }

    // The connection is shared when an enclosing <cftransaction> opened one.
    TxFrame *tx = transaction_get_active(dsn);
    db::DBConnection *conn = nullptr;
    if (tx && tx->conn) {
        conn = tx->conn;
    } else {
        conn = db::getConnection(dsn, 0);
        if (tx) {
            tx->conn = conn;
            tx->inTransaction = true;
            conn->begin();
        }
    }

    if (webstrada::config::enableQueryLogging) {
        printf("[cfstoredproc] dsn=%s proc=%s params=%zu\n", dsn.c_str(), procedure.c_str(), dbParams.size());
        fflush(stdout);
    }

    auto execStart = std::chrono::steady_clock::now();
    db::DBStoredProcResult sp = conn->storedProc(procedure, dbParams);
    auto execEnd = std::chrono::steady_clock::now();
    long long execTime = std::chrono::duration_cast<std::chrono::milliseconds>(execEnd - execStart).count();

    if (webstrada::config::enableQueryLogging) {
        printf("[cfstoredproc]   -> %zu result sets, %zu out params in %lldms\n",
               sp.resultsets.size(), sp.outValues.size(), execTime);
        fflush(stdout);
    }

    // Bind the result sets to the <cfprocresult> names, in resultset order
    // (later declarations of the same index win, like CF's TreeMap).
    std::stable_sort(ctx.results.begin(), ctx.results.end(),
        [](const StoredProcResultBinding &a, const StoredProcResultBinding &b) {
            return a.resultset < b.resultset;
        });
    for (auto &b : ctx.results) {
        size_t idx = static_cast<size_t>(b.resultset - 1);
        if (idx >= sp.resultsets.size()) continue; // not returned: leave unbound
        cfvariant q = queryFromDbResult(limitRows(sp.resultsets[idx], b.maxrows));
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         b.name.c_str(), &q);
    }

    // Assign the out/inout parameter values to their `variable` names.
    size_t outIdx = 0;
    for (auto &p : ctx.params) {
        if (p.type != "out" && p.type != "inout") continue;
        std::string outVal = outIdx < sp.outValues.size() ? sp.outValues[outIdx] : "";
        outIdx++;
        if (p.variable.empty()) continue;
        if (p.maxlength >= 0 && outVal.size() > static_cast<size_t>(p.maxlength)) {
            outVal = trimString(outVal.substr(0, static_cast<size_t>(p.maxlength)));
        }
        cfvariant v(outVal.c_str());
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         p.variable.c_str(), &v);
    }

    long long statusCode = sp.hasStatusCode ? sp.statusCode : 0;

    if (!resultName.empty()) {
        // result="name": a struct with EXECUTIONTIME / CACHED / STATUSCODE.
        cfvariant resultStruct(cfvariant::Struct);
        cfvariant timeVal(cfvariant::Long);
        timeVal.m_long = execTime;
        resultStruct.set("EXECUTIONTIME") = timeVal;
        resultStruct.set("CACHED") = cfvariant("false");
        if (returncode) {
            cfvariant codeVal(cfvariant::Long);
            codeVal.m_long = statusCode;
            resultStruct.set("STATUSCODE") = codeVal;
        }
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         resultName.c_str(), &resultStruct);
    } else {
        // No result attribute: cfstoredproc.executiontime / .statuscode page
        // variables (CF's "CFSTOREDPROC.ExecutionTime" / "CFSTOREDPROC.StatusCode").
        cfvariant timeVal(cfvariant::Long);
        timeVal.m_long = execTime;
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         "cfstoredproc.ExecutionTime", &timeVal);
        if (returncode) {
            cfvariant codeVal(cfvariant::Long);
            codeVal.m_long = statusCode;
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             "cfstoredproc.StatusCode", &codeVal);
        }
    }
}

} // namespace cfml

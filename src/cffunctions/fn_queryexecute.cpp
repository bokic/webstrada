/**
 * @file fn_queryexecute.cpp
 * @brief CFML queryExecute() built-in.
 *
 * CF's QueryExecute parses the SQL for `?` (positional) or `:name` (named)
 * placeholders outside quotes/comments, then binds each parameter to the JDBC
 * statement. This engine's <cfquery> runs raw SQL text against SQLite (it has
 * no parameter binding layer), so queryExecute() substitutes each parameter as
 * an inline SQL literal (quote-escaped strings, raw numbers, NULL, ISO dates)
 * and hands the resulting text to the shared query executor (cf_run_query).
 * Parameter structs accept the same keys as <cfqueryparam>: value, cfsqltype,
 * null, list, separator, maxlength, scale.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

namespace cfml {

using webstrada::cfvariant;
using webstrada::string;

namespace {

// Format a single parameter value as an inline SQL literal.
std::string formatSqlLiteral(const cfvariant *v, bool isNull)
{
    if (!v || isNull) return "NULL";
    switch (v->m_type) {
        case cfvariant::Number:
        case cfvariant::Long:
            return safe_to_std_string(*v);
        case cfvariant::Float: {
            std::string s = safe_to_std_string(*v);
            return s;
        }
        case cfvariant::Boolean:
            return v->m_bool ? "1" : "0";
        case cfvariant::Binary: {
            std::string out = "X'";
            if (v->m_binary) {
                char hex[3];
                for (auto b : *v->m_binary) {
                    std::snprintf(hex, sizeof(hex), "%02x", static_cast<unsigned char>(b));
                    out += hex;
                }
            }
            out += "'";
            return out;
        }
        case cfvariant::DateTime: {
            struct tm tmv = daysToTm(v->m_double);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                          tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                          tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
            std::string out = "'";
            out += buf;
            out += "'";
            return out;
        }
        default: {
            std::string s = safe_to_std_string(*v);
            double days = 0;
            if (parseDateTimeStr(s.c_str(), days)) {
                struct tm tmv = daysToTm(days);
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                              tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                              tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
                std::string out = "'";
                out += buf;
                out += "'";
                return out;
            }
            std::string out = "'";
            for (char c : s) {
                if (c == '\'') out += "''";
                else out += c;
            }
            out += "'";
            return out;
        }
    }
}

// Substitute positional `?` placeholders (CF's QueryUtils.processQueryParam).
std::string substitutePositional(const std::string &sql, const cfvariant *params)
{
    if (!params || params->m_type != cfvariant::Array || !params->m_array) return sql;
    const auto &arr = *params->m_array;
    std::string out;
    size_t len = sql.size();
    bool inSingle = false, inDouble = false, inComment = false;
    size_t paramIdx = 0;
    for (size_t i = 0; i < len; i++) {
        char c = sql[i];
        if (!inSingle && !inDouble && i > 0 && sql[i - 1] == '-' && c == '-') inComment = true;
        if (!inSingle && !inDouble && (c == '\n' || (c == '\r' && inComment))) inComment = false;
        if (inComment) { out += c; continue; }
        if (inSingle) {
            out += c;
            if (c == '\'') inSingle = false;
            continue;
        }
        if (inDouble) {
            out += c;
            if (c == '"') inDouble = false;
            continue;
        }
        if (c == '\'') { inSingle = true; out += c; continue; }
        if (c == '"') { inDouble = true; out += c; continue; }
        if (c == '?') {
            if (paramIdx < arr.size()) {
                const cfvariant &p = arr[paramIdx];
                paramIdx++;
                bool isNull = false;
                const cfvariant *value = &p;
                if (p.m_type == cfvariant::Struct && p.m_struct) {
                    auto git = p.m_struct->find(string("NULL"));
                    if (git != p.m_struct->end() && cfvariant_is_truthy(&git->second)) isNull = true;
                    auto vit = p.m_struct->find(string("VALUE"));
                    if (vit != p.m_struct->end()) value = &vit->second;
                    auto lit = p.m_struct->find(string("LIST"));
                    bool isList = (lit != p.m_struct->end()) && cfvariant_is_truthy(&lit->second);
                    if (isList) {
                        auto sit = p.m_struct->find(string("SEPARATOR"));
                        std::string sep = (sit != p.m_struct->end()) ? safe_to_std_string(sit->second) : ",";
                        std::string listStr = safe_to_std_string(*value);
                        size_t start = 0, pos;
                        std::vector<std::string> toks;
                        while ((pos = listStr.find(sep, start)) != std::string::npos) {
                            toks.push_back(listStr.substr(start, pos - start));
                            start = pos + sep.size();
                        }
                        toks.push_back(listStr.substr(start));
                        for (size_t t = 0; t < toks.size(); t++) {
                            if (t > 0) out += ',';
                            cfvariant tv(toks[t].c_str());
                            out += formatSqlLiteral(&tv, false);
                        }
                        continue;
                    }
                }
                out += formatSqlLiteral(value, isNull);
                continue;
            }
            out += c;
            continue;
        }
        out += c;
    }
    return out;
}

// Substitute named `:name` parameters (CF's convertQueryParamsMapToList).
std::string substituteParams(const std::string &sql, const cfvariant *params)
{
    if (!params) return sql;
    if (params->m_type != cfvariant::Struct || !params->m_struct) return sql;

    std::string out;
    size_t len = sql.size();
    bool inSingle = false, inDouble = false, inComment = false;
    size_t paramIdx = 0;
    for (size_t i = 0; i < len; i++) {
        char c = sql[i];
        if (!inSingle && !inDouble && i > 0 && sql[i - 1] == '-' && c == '-') inComment = true;
        if (!inSingle && !inDouble && (c == '\n' || (c == '\r' && inComment))) inComment = false;
        if (inComment) { out += c; continue; }
        if (inSingle) {
            out += c;
            if (c == '\'') inSingle = false;
            continue;
        }
        if (inDouble) {
            out += c;
            if (c == '"') inDouble = false;
            continue;
        }
        if (c == '\'') { inSingle = true; out += c; continue; }
        if (c == '"') { inDouble = true; out += c; continue; }
        if (c == ':' && i + 1 < len) {
            if (sql[i + 1] == ':') { out += c; out += sql[i + 1]; i++; continue; }
            if (isalpha((unsigned char)sql[i + 1])) {
                size_t j = i + 1;
                while (j < len && (isalnum((unsigned char)sql[j]) || sql[j] == '_')) j++;
                std::string name = sql.substr(i + 1, j - i - 1);
                string key(name.c_str());
                auto it = params->m_struct->find(key);
                if (it != params->m_struct->end()) {
                    const cfvariant &p = it->second;
                    bool isNull = false;
                    const cfvariant *value = &p;
                    if (p.m_type == cfvariant::Struct && p.m_struct) {
                        auto git = p.m_struct->find(string("NULL"));
                        if (git != p.m_struct->end() && cfvariant_is_truthy(&git->second)) isNull = true;
                        auto vit = p.m_struct->find(string("VALUE"));
                        if (vit != p.m_struct->end()) value = &vit->second;
                        auto lit = p.m_struct->find(string("LIST"));
                        bool isList = (lit != p.m_struct->end()) && cfvariant_is_truthy(&lit->second);
                        if (isList) {
                            auto sit = p.m_struct->find(string("SEPARATOR"));
                            std::string sep = (sit != p.m_struct->end()) ? safe_to_std_string(sit->second) : ",";
                            std::string listStr = safe_to_std_string(*value);
                            size_t start = 0, pos;
                            std::vector<std::string> toks;
                            while ((pos = listStr.find(sep, start)) != std::string::npos) {
                                toks.push_back(listStr.substr(start, pos - start));
                                start = pos + sep.size();
                            }
                            toks.push_back(listStr.substr(start));
                            for (size_t t = 0; t < toks.size(); t++) {
                                if (t > 0) out += ',';
                                cfvariant tv(toks[t].c_str());
                                out += formatSqlLiteral(&tv, false);
                            }
                            i = j - 1;
                            continue;
                        }
                    }
                    out += formatSqlLiteral(value, isNull);
                    i = j - 1;
                    continue;
                }
                out += c;
                continue;
            }
        }
        out += c;
    }
    return out;
}

} // namespace

cfvariant *cf_queryexecute(const cfvariant *sqlArg, const cfvariant *params, const cfvariant *options,
                           void *cgi, void *server, void *cookie, void *application,
                           void *session, void *url, void *form, void *variables)
{
    if (!sqlArg) throw webstrada::exception("QueryExecute requires at least 1 argument");
    std::string sql = safe_to_std_string(*sqlArg);

    // CF accepts a comma-separated "datasource=..." inline prefix too; the
    // engine handles only the (sql [, params [, options]]) form, matching the
    // docs. Params are substituted into the SQL text as inline literals.
    std::string finalSql;
    if (params && params->m_type == cfvariant::Struct && params->m_struct) {
        finalSql = substituteParams(sql, params);
    } else {
        finalSql = substitutePositional(sql, params);
    }

    // Build the attrs struct from the options (all cfquery attributes except
    // name are supported, per the CF docs).
    cfvariant attrs(cfvariant::Struct);
    if (options && options->m_type == cfvariant::Struct && options->m_struct) {
        for (const auto &kv : *options->m_struct) {
            attrs.structSet(kv.first, kv.second);
        }
    }
    if (!attrs.m_struct || attrs.m_struct->find(string("datasource")) == attrs.m_struct->end()) {
        attrs.structSet(string("datasource"), cfvariant("webstrada"));
    }

    cfvariant *result = cf_run_query(finalSql, &attrs, cgi, server, cookie, application, session, url, form, variables);
    return result;
}

} // namespace cfml

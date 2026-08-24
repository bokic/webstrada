/**
 * @file tag_queryparam.cpp
 * @brief <cfqueryparam> runtime (cf_queryparam).
 *
 * This engine has no JDBC bind layer, so <cfqueryparam> substitutes the value
 * as an inline SQL literal into the <cfquery> capture buffer (the same
 * approach queryExecute() uses). Type coercion reproduces CF's QueryParamTag
 * validation: integer/numeric/decimal/date/string types, maxlength, scale,
 * null and list expansion.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>

namespace cfml {

namespace {

// Normalize a CFSQLTYPE name: uppercase, optional "CF_SQL_" prefix stripped.
std::string normalizeSqlType(const std::string &t)
{
    std::string up;
    for (char c : t) up += static_cast<char>(toupper((unsigned char)c));
    if (up.rfind("CF_SQL_", 0) == 0) up = up.substr(7);
    return up;
}

// CF's canonical CFSQLTYPE name (used in "Invalid data X for CFSQLTYPE Y.").
std::string canonicalSqlType(const std::string &t)
{
    return "CF_SQL_" + normalizeSqlType(t);
}

bool isIntegerType(const std::string &t)
{
    return t == "INTEGER" || t == "BIGINT" || t == "SMALLINT" || t == "TINYINT";
}

bool isNumericType(const std::string &t)
{
    return t == "NUMERIC" || t == "DECIMAL" || t == "FLOAT" || t == "DOUBLE" ||
           t == "REAL" || t == "MONEY" || t == "MONEY4";
}

bool isStringType(const std::string &t)
{
    return t == "CHAR" || t == "VARCHAR" || t == "LONGVARCHAR";
}

bool isDateType(const std::string &t)
{
    return t == "DATE" || t == "TIME" || t == "TIMESTAMP";
}

bool isBooleanType(const std::string &t)
{
    return t == "BIT";
}

// Format a single parameter value as an inline SQL literal, coercing it to
// `sqlType` like CF's QueryParamTag. Throws CF's validation errors.
std::string formatQueryParamValue(const cfvariant *v, const std::string &sqlType,
                                  long long maxlength, int scale)
{
    if (!v || v->m_type == cfvariant::Null) return "NULL";
    std::string s = safe_to_std_string(*v);

    if (maxlength >= 0 && static_cast<long long>(s.size()) > maxlength) {
        webstrada::string msg("Invalid data value ");
        msg.append(s.c_str());
        msg.append(" exceeds maxlength setting ");
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%lld", (long long)maxlength);
        msg.append(buf);
        msg.append(".");
        throw webstrada::exception(msg);
    }

    // Numeric value of a cfvariant (int/long/float), used for the integer and
    // numeric type branches without re-parsing the string.
    auto doubleOf = [&](double &out) -> bool {
        if (v->m_type == cfvariant::Number) { out = (double)v->m_int; return true; }
        if (v->m_type == cfvariant::Long)   { out = (double)v->m_long; return true; }
        if (v->m_type == cfvariant::Float)  { out = v->m_double; return true; }
        return false;
    };

    if (isIntegerType(sqlType)) {
        double d = 0;
        if (!doubleOf(d)) {
            const char *p = s.c_str();
            char *end = nullptr;
            d = strtod(p, &end);
            if (end == p || *end != '\0') {
                throw webstrada::exception(("Invalid data " + s + " for CFSQLTYPE " + canonicalSqlType(sqlType) + ".").c_str());
            }
        }
        if (std::isnan(d) || std::isinf(d)) {
            throw webstrada::exception(("Invalid data " + s + " for CFSQLTYPE " + canonicalSqlType(sqlType) + ".").c_str());
        }
        long long iv = (long long)d;
        return std::to_string(iv);
    }

    if (isNumericType(sqlType)) {
        double d = 0;
        if (!doubleOf(d)) {
            const char *p = s.c_str();
            char *end = nullptr;
            d = strtod(p, &end);
            if (end == p || *end != '\0') {
                throw webstrada::exception(("Invalid data " + s + " for CFSQLTYPE " + canonicalSqlType(sqlType) + ".").c_str());
            }
        }
        if (std::isnan(d) || std::isinf(d)) {
            throw webstrada::exception(("Invalid data " + s + " for CFSQLTYPE " + canonicalSqlType(sqlType) + ".").c_str());
        }
        char buf[64];
        if (sqlType == "NUMERIC" || sqlType == "DECIMAL") {
            std::snprintf(buf, sizeof(buf), "%.*f", scale, d);
        } else {
            std::snprintf(buf, sizeof(buf), "%.10g", d);
        }
        return buf;
    }

    if (isBooleanType(sqlType)) {
        webstrada::string low = s.c_str();
        low.toLower();
        if (low.equals("true") || low.equals("yes") || low.equals("1")) return "1";
        if (low.equals("false") || low.equals("no") || low.equals("0")) return "0";
        throw webstrada::exception(("Invalid data " + s + " for CFSQLTYPE " + canonicalSqlType(sqlType) + ".").c_str());
    }

    if (isDateType(sqlType)) {
        // A DateTime variant or valid date string renders standard SQL format 'YYYY-MM-DD HH:MM:SS'
        // or 'YYYY-MM-DD' / 'HH:MM:SS'. Throws CF's validation error if invalid.
        if (v->m_type == cfvariant::DateTime) {
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
        double days = 0;
        if (!parseDateTimeStr(s.c_str(), days)) {
            throw webstrada::exception((s + " is an invalid date or time string.").c_str());
        }
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

    if (isStringType(sqlType) || sqlType == "IDSTAMP") {
        std::string out = "'";
        for (char c : s) {
            if (c == '\'') out += "''";
            else out += c;
        }
        out += "'";
        return out;
    }

    if (sqlType == "BINARY" || sqlType == "VARBINARY" || sqlType == "LONGVARBINARY") {
        std::string out = "X'";
        if (v->m_type == cfvariant::Binary && v->m_binary) {
            char hex[3];
            for (auto b : *v->m_binary) {
                std::snprintf(hex, sizeof(hex), "%02x", static_cast<unsigned char>(b));
                out += hex;
            }
        } else {
            for (unsigned char c : s) {
                char hex[3];
                std::snprintf(hex, sizeof(hex), "%02x", c);
                out += hex;
            }
        }
        out += "'";
        return out;
    }

    // Unknown/unsupported CFSQLTYPE: fall back to the string form (CF treats
    // the value as its default CHAR).
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "''";
        else out += c;
    }
    out += "'";
    return out;
}

} // namespace

void cf_queryparam(void *out, const cfvariant *value, const cfvariant *cfsqltype,
                   const cfvariant *maxlength, const cfvariant *scale,
                   const cfvariant *null, const cfvariant *list,
                   const cfvariant *separator)
{
    string *outStr = static_cast<string*>(out);

    std::string sqlType = cfsqltype ? safe_to_std_string(*cfsqltype) : "CF_SQL_CHAR";
    sqlType = normalizeSqlType(sqlType);
    if (sqlType.empty()) sqlType = "CHAR";

    long long maxlen = -1;
    if (maxlength) maxlen = cfvariant_to_long(maxlength);
    int sc = 0;
    if (scale) sc = (int)cfvariant_to_long(scale);

    // null="true" ignores value and passes NULL.
    if (null && cfvariant_is_truthy(null)) {
        outStr->append("NULL");
        return;
    }

    // list="true": the value is a delimited list; expand each element.
    if (list && cfvariant_is_truthy(list)) {
        std::string sep = separator ? safe_to_std_string(*separator) : ",";
        std::string s = value ? safe_to_std_string(*value) : "";
        std::string result;
        size_t start = 0, pos;
        bool firstTok = true;
        while ((pos = s.find(sep, start)) != std::string::npos) {
            std::string tok = s.substr(start, pos - start);
            if (!firstTok) result += ",";
            cfvariant tv(tok.c_str());
            result += formatQueryParamValue(&tv, sqlType, maxlen, sc);
            firstTok = false;
            start = pos + sep.size();
        }
        std::string tok = s.substr(start);
        if (!firstTok) result += ",";
        cfvariant tv(tok.c_str());
        result += formatQueryParamValue(&tv, sqlType, maxlen, sc);
        outStr->append(result.c_str());
        return;
    }

    std::string lit = formatQueryParamValue(value, sqlType, maxlen, sc);
    outStr->append(lit.c_str());
}

} // namespace cfml

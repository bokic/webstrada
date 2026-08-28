/**
 * @file tag_wddx.cpp
 * @brief <cfwddx> runtime (cf_wddx_tag).
 *
 * Implements the four <cfwddx> actions with Adobe ColdFusion 2025 semantics
 * (ported from the decompiled WddxTag / WddxSerializer / WddxDeserializer /
 * StringFormatter.scriptFormat):
 *   cfml2wddx — serialize a CFML value to a WDDX 1.0 packet,
 *   wddx2cfml — deserialize a WDDX packet to a CFML value,
 *   cfml2js   — render a CFML value as JavaScript assignment statements,
 *   wddx2js   — deserialize a WDDX packet, then render it as JavaScript.
 * The result is stored in the `output` attribute variable (when given) or
 * written to the page. The serialized packet format, the char escaping tables,
 * the number/datetime rendering and the JavaScript layout are all byte-verified
 * against CF 2025 on the RDS host (tests/cfm/cfwddx_test.cfm).
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include "../cffunctions/common.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <charconv>

#include <algorithm>
#include <cctype>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

using webstrada::cfvariant;
using webstrada::string;

// Read an attribute from the evaluated attribute struct (case-insensitive).
const cfvariant *attrOf(const cfvariant *attrs, const char *key)
{
    if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
    string k(key);
    auto it = attrs->m_struct->find(k);
    return it == attrs->m_struct->end() ? nullptr : &it->second;
}

std::string attrStr(const cfvariant *attrs, const char *key)
{
    const cfvariant *v = attrOf(attrs, key);
    return v ? cfml::safe_to_std_string(*v) : std::string();
}

// ---- Java Double.toString replica (used for <number> content) ----

// Expand a decimal mantissa+exponent (from std::to_chars shortest) into the
// fixed-point form Java's Double.toString uses for exponents in [-3, 7).
static std::string fixedFromMantissaExp(const std::string &mant, int exp)
{
    bool neg = !mant.empty() && mant[0] == '-';
    std::string digits;
    std::string::size_type dot = mant.find('.');
    if (dot == std::string::npos) {
        digits = neg ? mant.substr(1) : mant;
    } else {
        digits = mant.substr(neg ? 1 : 0, dot - (neg ? 1 : 0)) + mant.substr(dot + 1);
    }
    // Remove trailing zeros of the fractional part for the fixed form.
    // The value is digits * 10^exp (mantissa "D.DDD" -> digits "DDDDD").
    int pointPos = static_cast<int>(digits.size()) + exp;  // digits before the point
    std::string intPart, fracPart;
    if (pointPos <= 0) {
        intPart = "0";
        fracPart = std::string(static_cast<size_t>(-pointPos), '0') + digits;
    } else if (pointPos >= static_cast<int>(digits.size())) {
        intPart = digits + std::string(static_cast<size_t>(pointPos - digits.size()), '0');
        fracPart = "";
    } else {
        intPart = digits.substr(0, static_cast<size_t>(pointPos));
        fracPart = digits.substr(static_cast<size_t>(pointPos));
    }
    if (fracPart.empty()) {
        return (neg ? "-" : "") + intPart + ".0";
    }
    return (neg ? "-" : "") + intPart + "." + fracPart;
}

// Java Double.toString (String.valueOf(double)) for the values <cfwddx> emits:
// the shortest round-trip decimal, with ".0" appended to whole values, and
// scientific notation ("1.0E10") only when the decimal exponent is < -3 or >= 7.
std::string javaDoubleToString(double d)
{
    if (std::isnan(d)) return "NaN";
    if (std::isinf(d)) return d > 0 ? "Infinity" : "-Infinity";
    if (d == 0.0) return "0.0";
    char buf[128];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), d);
    if (ec != std::errc()) return "0.0";
    std::string s(buf, ptr - buf);
    auto epos = s.find_first_of("eE");
    if (epos == std::string::npos) {
        // fixed-point shortest repr.
        if (s.find('.') == std::string::npos) s += ".0";
        return s;
    }
    int decExp = static_cast<int>(std::floor(std::log10(std::fabs(d))));
    std::string mant = s.substr(0, epos);
    int exp = std::atoi(s.c_str() + epos + 1);
    if (decExp < -3 || decExp >= 7) {
        if (mant.find('.') == std::string::npos) mant += ".0";
        char eb[32];
        std::snprintf(eb, sizeof(eb), "%d", exp);
        return mant + "E" + eb;
    }
    return fixedFromMantissaExp(mant, exp);
}

// ---- WDDX char escaping (UTF8Converter) ----

// Writes one character according to the WDDX char tables. `attr` selects the
// attribute table (which additionally escapes ' and ").
static void appendWddxChar(std::string &out, int c, bool attr)
{
    if (c < 32) {
        char tmp[32];
        std::snprintf(tmp, sizeof(tmp), "<char code='%02x'/>", c);
        out += tmp;
        return;
    }
    if (c == 60) { out += "&lt;"; return; }
    if (c == 62) { out += "&gt;"; return; }
    if (c == 38) { out += "&amp;"; return; }
    if (attr && c == 39) { out += "&apos;"; return; }
    if (attr && c == 34) { out += "&quot;"; return; }
    if (c >= 128 && c < 256) {
        char tmp[16];
        std::snprintf(tmp, sizeof(tmp), "&#x%02x;", c);
        out += tmp;
        return;
    }
    if (c >= 256) {
        char tmp[32];
        std::snprintf(tmp, sizeof(tmp), "&#x%x;", c);
        out += tmp;
        return;
    }
    out += static_cast<char>(c);
}

// Escapes a UTF-8 byte stream per the WDDX char table (iterating over the
// UTF-8 code points).
static std::string wddxEscape(const std::string &s, bool attr)
{
    std::string out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int cp = c;
        size_t len = 1;
        if (c >= 0xF0 && i + 3 < s.size()) { cp = ((c & 0x07) << 18) | ((s[i+1] & 0x3F) << 12) | ((s[i+2] & 0x3F) << 6) | (s[i+3] & 0x3F); len = 4; }
        else if (c >= 0xE0 && i + 2 < s.size()) { cp = ((c & 0x0F) << 12) | ((s[i+1] & 0x3F) << 6) | (s[i+2] & 0x3F); len = 3; }
        else if (c >= 0xC0 && i + 1 < s.size()) { cp = ((c & 0x1F) << 6) | (s[i+1] & 0x3F); len = 2; }
        appendWddxChar(out, cp, attr);
        i += len;
    }
    return out;
}

// ---- Java-style JS string escaping (StringFormatter.escapeJSString) ----
static std::string escapeJSString(const std::string &s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        switch (c) {
            case '\b': out += "\\b"; break;
            case '\t': out += "\\t"; break;
            case '\n': out += "\\n"; break;
            case '\f': out += "\\f"; break;
            case '\r': out += "\\r"; break;
            case '\"': out += "\\\""; break;
            case '\'': out += "\\'"; break;
            case '\\': out += "\\\\"; break;
            default: out += c; break;
        }
    }
    return out;
}

// ---- struct key ordering (CF's java.util.HashMap iteration order) ----

std::vector<std::string> structKeyOrder(const cfvariant &st)
{
    std::vector<std::string> keys;
    if (!st.m_struct) return keys;
    for (const auto &kv : *st.m_struct) {
        if (kv.first.constData()) keys.push_back(kv.first.constData());
    }
    if (st.m_serializeInsertOrder) {
        std::vector<std::string> ordered;
        ordered.reserve(keys.size());
        if (st.m_structInsertOrder) {
            for (const auto &k : *st.m_structInsertOrder) {
                const char *c = k.constData();
                if (c && st.m_struct->count(c) &&
                    std::find(ordered.begin(), ordered.end(), c) == ordered.end()) {
                    ordered.push_back(c);
                }
            }
        }
        for (const auto &kv : *st.m_struct) {
            const char *c = kv.first.constData();
            if (c && std::find(ordered.begin(), ordered.end(), c) == ordered.end()) {
                ordered.push_back(c);
            }
        }
        return ordered;
    }
    std::map<std::string, size_t> insOrder;
    if (st.m_structInsertOrder) {
        for (size_t i = 0; i < st.m_structInsertOrder->size(); i++) {
            const char *k = st.m_structInsertOrder->at(i).constData();
            if (k) insOrder.emplace(k, i);
        }
    }
    int cap = cfml::javaHashMapCapacity(st.m_struct->size());
    std::stable_sort(keys.begin(), keys.end(), [&](const std::string &a, const std::string &b) {
        int ba = cfml::javaHashMapBucket(a.c_str(), cap);
        int bb = cfml::javaHashMapBucket(b.c_str(), cap);
        if (ba != bb) return ba < bb;
        auto ia = insOrder.find(a);
        auto ib = insOrder.find(b);
        size_t sa = (ia != insOrder.end()) ? ia->second : (size_t)-1;
        size_t sb = (ib != insOrder.end()) ? ib->second : (size_t)-1;
        if (sa != sb) return sa < sb;
        return a < b;
    });
    return keys;
}

// ---- date helpers ----

// Local timezone offset (seconds east of UTC) at the instant described by a CF
// DateTime value (days since 1899-12-30, stored as the local wall-clock).
int localOffsetSeconds(double days)
{
    time_t t = static_cast<time_t>((days - 25569.0) * 86400.0);
    struct tm tmv;
    memset(&tmv, 0, sizeof(tmv));
    localtime_r(&t, &tmv);
    return static_cast<int>(tmv.tm_gmtoff);
}

// Renders a CF DateTime as the WDDX <dateTime> content (ISO8601.stringValueOf).
std::string wddxDateTime(double days, bool useTimezone)
{
    struct tm tm = cfml::daysToTm(days);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    std::string out(buf);
    if (useTimezone) {
        int off = localOffsetSeconds(days);
        int mins = off / 60;
        char ob[16];
        std::snprintf(ob, sizeof(ob), "%s%02d:%02d", mins < 0 ? "-" : "+",
                      std::abs(mins) / 60, std::abs(mins) % 60);
        out += ob;
    }
    return out;
}

// ---- serializer ----

void serializeWddxValue(const cfvariant &v, std::string &out, bool useTimezone);

void serializeWddxStruct(const cfvariant &st, std::string &out, bool useTimezone)
{
    if (st.m_serializeInsertOrder) {
        out += "<struct type='ordered'>";
    } else {
        out += "<struct>";
    }
    std::vector<std::string> keys = structKeyOrder(st);
    for (const auto &k : keys) {
        out += "<var name='";
        out += wddxEscape(k, true);
        out += "'>";
        auto it = st.m_struct->find(k.c_str());
        if (it != st.m_struct->end()) {
            serializeWddxValue(it->second, out, useTimezone);
        } else {
            out += "<null/>";
        }
        out += "</var>";
    }
    out += "</struct>";
}

void serializeWddxQuery(const cfvariant &q, std::string &out, bool useTimezone)
{
    webstrada::QueryData *qd = q.m_query;
    int rows = qd ? qd->rowCount() : 0;
    std::string fields;
    if (qd) {
        for (size_t c = 0; c < qd->columns.size(); c++) {
            if (c) fields += ",";
            fields += qd->columns[c].name.constData() ? qd->columns[c].name.constData() : "";
        }
    }
    out += "<recordset rowCount='";
    out += std::to_string(rows);
    out += "' fieldNames='";
    out += fields;
    out += "' type='coldfusion.sql.QueryTable'>";
    if (qd) {
        for (const auto &col : qd->columns) {
            out += "<field name='";
            out += wddxEscape(col.name.constData() ? col.name.constData() : "", true);
            out += "'>";
            for (const auto &cell : col.values) {
                serializeWddxValue(cell, out, useTimezone);
            }
            out += "</field>";
        }
    }
    out += "</recordset>";
}

void serializeWddxValue(const cfvariant &v, std::string &out, bool useTimezone)
{
    switch (v.m_type) {
        case cfvariant::Null:
            out += "<null/>";
            break;
        case cfvariant::Boolean:
            out += v.m_bool ? "<boolean value='true'/>" : "<boolean value='false'/>";
            break;
        case cfvariant::Number:
            out += "<number>";
            out += javaDoubleToString(static_cast<double>(v.m_int));
            out += "</number>";
            break;
        case cfvariant::Long:
            out += "<number>";
            out += javaDoubleToString(static_cast<double>(v.m_long));
            out += "</number>";
            break;
        case cfvariant::Float:
            out += "<number>";
            out += javaDoubleToString(v.m_double);
            out += "</number>";
            break;
        case cfvariant::String:
            out += "<string>";
            out += wddxEscape(v.m_str ? v.m_str->constData() : "", false);
            out += "</string>";
            break;
        case cfvariant::DateTime:
            out += "<dateTime>";
            out += wddxDateTime(v.m_double, useTimezone);
            out += "</dateTime>";
            break;
        case cfvariant::Array:
            if (v.m_queryColOwner) {
                // A query-column reference degrades to its scalar first cell.
                serializeWddxValue(cfml::queryColumnFirstCell(&v), out, useTimezone);
                break;
            }
            out += "<array length='";
            out += std::to_string(v.m_array ? v.m_array->size() : 0);
            out += "'>";
            if (v.m_array) {
                for (const auto &el : *v.m_array) serializeWddxValue(el, out, useTimezone);
            }
            out += "</array>";
            break;
        case cfvariant::Struct:
            serializeWddxStruct(v, out, useTimezone);
            break;
        case cfvariant::Query:
            serializeWddxQuery(v, out, useTimezone);
            break;
        default:
            // Everything else (components, images, functions, files, ...) CF
            // cannot serialize: WddxOutputStream throws "cannot serialize
            // object type". Emit an empty string like the fallback path.
            out += "<string></string>";
            break;
    }
}

// ---- deserializer (libxml2 tree walk) ----

void deserializeWddxElement(xmlNodePtr node, cfvariant &out);

// The character content of a <string> node (including <char code='..'/>).
static std::string stringContentOf(xmlNodePtr node)
{
    std::string res;
    for (xmlNodePtr ch = node->children; ch; ch = ch->next) {
        if (ch->type == XML_TEXT_NODE || ch->type == XML_CDATA_SECTION_NODE) {
            xmlChar *c = xmlNodeGetContent(ch);
            if (c) { res += reinterpret_cast<const char*>(c); xmlFree(c); }
        } else if (ch->type == XML_ELEMENT_NODE &&
                   xmlStrcmp(ch->name, BAD_CAST "char") == 0) {
            xmlChar *code = xmlGetProp(ch, BAD_CAST "code");
            if (code) {
                char *end = nullptr;
                long cp = strtol(reinterpret_cast<const char*>(code), &end, 16);
                xmlFree(code);
                if (end && *end == 0 && cp >= 0) {
                    // Encode the code point back to UTF-8.
                    unsigned int u = static_cast<unsigned int>(cp);
                    if (u < 0x80) {
                        res += static_cast<char>(u);
                    } else if (u < 0x800) {
                        res += static_cast<char>(0xC0 | (u >> 6));
                        res += static_cast<char>(0x80 | (u & 0x3F));
                    } else if (u < 0x10000) {
                        res += static_cast<char>(0xE0 | (u >> 12));
                        res += static_cast<char>(0x80 | ((u >> 6) & 0x3F));
                        res += static_cast<char>(0x80 | (u & 0x3F));
                    } else {
                        res += static_cast<char>(0xF0 | (u >> 18));
                        res += static_cast<char>(0x80 | ((u >> 12) & 0x3F));
                        res += static_cast<char>(0x80 | ((u >> 6) & 0x3F));
                        res += static_cast<char>(0x80 | (u & 0x3F));
                    }
                }
            }
        }
    }
    return res;
}

static cfvariant structFromWddx(xmlNodePtr node)
{
    cfvariant st(cfvariant::Struct);
    for (xmlNodePtr ch = node->children; ch; ch = ch->next) {
        if (ch->type != XML_ELEMENT_NODE || xmlStrcmp(ch->name, BAD_CAST "var") != 0) continue;
        xmlChar *name = xmlGetProp(ch, BAD_CAST "name");
        if (!name) continue;
        std::string key(reinterpret_cast<const char*>(name));
        xmlFree(name);
        // Find the single typed child element; bare-text vars are skipped
        // (CF's StructHandler only stores a var when a child element completes).
        xmlNodePtr valNode = nullptr;
        for (xmlNodePtr cc = ch->children; cc; cc = cc->next) {
            if (cc->type == XML_ELEMENT_NODE) { valNode = cc; break; }
        }
        if (!valNode) continue;
        cfvariant val;
        deserializeWddxElement(valNode, val);
        st.set(key.c_str()) = val;
    }
    return st;
}

static cfvariant arrayFromWddx(xmlNodePtr node)
{
    cfvariant arr(cfvariant::Array);
    if (!arr.m_array) { arr.m_array = new std::vector<cfvariant>(); }
    for (xmlNodePtr ch = node->children; ch; ch = ch->next) {
        if (ch->type != XML_ELEMENT_NODE) continue;
        cfvariant el;
        deserializeWddxElement(ch, el);
        arr.insert(el);
    }
    return arr;
}

static cfvariant queryFromWddx(xmlNodePtr node)
{
    cfvariant q(cfvariant::Query);
    webstrada::QueryData *qd = q.m_query;
    xmlChar *rowCountStr = xmlGetProp(node, BAD_CAST "rowCount");
    int rows = rowCountStr ? std::atoi(reinterpret_cast<const char*>(rowCountStr)) : 0;
    if (rowCountStr) xmlFree(rowCountStr);
    for (xmlNodePtr ch = node->children; ch; ch = ch->next) {
        if (ch->type != XML_ELEMENT_NODE || xmlStrcmp(ch->name, BAD_CAST "field") != 0) continue;
        xmlChar *name = xmlGetProp(ch, BAD_CAST "name");
        std::string colName = name ? reinterpret_cast<const char*>(name) : "";
        if (name) xmlFree(name);
        webstrada::QueryColumn col;
        col.name = colName.c_str();
        col.type = "VARCHAR";
        for (xmlNodePtr cc = ch->children; cc; cc = cc->next) {
            if (cc->type != XML_ELEMENT_NODE) continue;
            cfvariant val;
            deserializeWddxElement(cc, val);
            col.values.push_back(val);
        }
        qd->columns.push_back(std::move(col));
    }
    qd->m_rowCount = rows;
    return q;
}

// Parses "2020-01-02T03:04:05[+-hh:mm]" into CF days (the local wall-clock
// representation, like CF's OleDateTime after ISO8601.parseDate).
bool parseWddxDateTime(const std::string &s, double &days)
{
    int year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0;
    int offHours = 0, offMins = 0;
    bool hasOffset = false;
    int negOffset = 1;
    int n = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d%n", &year, &mon, &day, &hour, &min, &sec, &n) < 6) {
        return false;
    }
    const char *rest = s.c_str() + n;
    if (*rest == '+' || *rest == '-') {
        hasOffset = true;
        if (*rest == '-') negOffset = -1;
        rest++;
        if (std::sscanf(rest, "%d:%d", &offHours, &offMins) < 2) return false;
    }
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = year - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = -1;
    time_t utc = timegm(&tm);
    if (utc == static_cast<time_t>(-1)) return false;
    if (hasOffset) {
        utc -= static_cast<time_t>(negOffset) * (offHours * 3600 + offMins * 60);
    }
    struct tm local;
    memset(&local, 0, sizeof(local));
    localtime_r(&utc, &local);
    days = cfml::tmToDays(local);
    return true;
}

static bool decodeBase64(const std::string &in, std::vector<std::byte> &out)
{
    static const char *tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char lut[256];
    memset(lut, 0xFF, sizeof(lut));
    for (int i = 0; i < 64; i++) lut[static_cast<unsigned char>(tbl[i])] = static_cast<unsigned char>(i);
    unsigned int acc = 0;
    int bits = 0;
    for (unsigned char c : in) {
        if (c == '=' || c == '\r' || c == '\n' || c == ' ' || c == '\t') {
            if (c == '=') break;
            continue;
        }
        if (lut[c] == 0xFF) return false;
        acc = (acc << 6) | lut[c];
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::byte>((acc >> bits) & 0xFF));
        }
    }
    return true;
}

void deserializeWddxElement(xmlNodePtr node, cfvariant &out)
{
    const char *name = reinterpret_cast<const char*>(node->name);
    if (strcmp(name, "null") == 0) {
        // CF's NullHandler: preserveNullValues off -> the empty string.
        out = cfvariant("");
    } else if (strcmp(name, "boolean") == 0) {
        xmlChar *val = xmlGetProp(node, BAD_CAST "value");
        bool b = val && strcmp(reinterpret_cast<const char*>(val), "true") == 0;
        if (val) xmlFree(val);
        cfvariant v(cfvariant::Boolean);
        v.m_bool = b;
        out = v;
    } else if (strcmp(name, "number") == 0) {
        std::string txt = stringContentOf(node);
        cfvariant v(cfvariant::Float);
        v.m_double = std::strtod(txt.c_str(), nullptr);
        out = v;
    } else if (strcmp(name, "string") == 0) {
        out = cfvariant(stringContentOf(node).c_str());
    } else if (strcmp(name, "dateTime") == 0) {
        std::string txt = stringContentOf(node);
        double days = 0;
        cfvariant v(cfvariant::DateTime);
        if (parseWddxDateTime(txt, days)) v.m_double = days;
        out = v;
    } else if (strcmp(name, "array") == 0) {
        out = arrayFromWddx(node);
    } else if (strcmp(name, "struct") == 0) {
        out = structFromWddx(node);
    } else if (strcmp(name, "recordset") == 0) {
        out = queryFromWddx(node);
    } else if (strcmp(name, "binary") == 0) {
        std::string txt = stringContentOf(node);
        std::vector<std::byte> bytes;
        if (decodeBase64(txt, bytes)) {
            cfvariant v(cfvariant::Binary);
            v.m_binary = new std::vector<std::byte>(std::move(bytes));
            out = v;
        } else {
            out = cfvariant("");
        }
    } else {
        // Unknown element: CF's ElementFactory throws; we keep it a null-ish
        // empty string so a <var> wrapping it is skipped.
        out = cfvariant("");
    }
}

// ---- JS rendering (StringFormatter.scriptFormat) ----

void renderJsValue(const cfvariant &v, const std::string &varName, std::string &out);

void renderJsStruct(const cfvariant &st, const std::string &varName, std::string &out)
{
    out += varName + " = new Object();\r\n";
    std::vector<std::string> keys = structKeyOrder(st);
    for (const auto &k : keys) {
        std::string esc;
        for (char c : k) {
            if (c == '\\') esc += "\\\\";
            else if (c == '\"') esc += "\\\"";
            else esc += c;
        }
        for (auto &c : esc) c = static_cast<char>(tolower((unsigned char)c));
        renderJsValue(st.m_struct->at(k.c_str()), varName + "[\"" + esc + "\"]", out);
    }
}

void renderJsQuery(const cfvariant &q, const std::string &varName, std::string &out)
{
    // wddxFormat=true: a WddxRecordset with per-column arrays.
    out += varName + " = new WddxRecordset();\r\n";
    webstrada::QueryData *qd = q.m_query;
    if (!qd) return;
    int colIdx = 0;
    for (const auto &col : qd->columns) {
        std::string colVar = "col" + std::to_string(colIdx);
        out += colVar + " = new Array();\r\n";
        for (int r = 0; r < static_cast<int>(col.values.size()); r++) {
            renderJsValue(col.values[r], colVar + "[" + std::to_string(r) + "]", out);
        }
        std::string lower;
        for (const char *c = col.name.constData(); c && *c; c++) lower += static_cast<char>(tolower((unsigned char)*c));
        out += varName + "[\"" + lower + "\"] = " + colVar + ";\r\n";
        out += colVar + " = null;\r\n";
        colIdx++;
    }
}

void renderJsArray(const cfvariant &arr, const std::string &varName, std::string &out)
{
    out += varName + " =  new Array();\r\n";
    if (arr.m_array) {
        for (size_t i = 0; i < arr.m_array->size(); i++) {
            renderJsValue((*arr.m_array)[i], varName + "[" + std::to_string(i) + "]", out);
        }
    }
}

void renderJsValue(const cfvariant &v, const std::string &varName, std::string &out)
{
    switch (v.m_type) {
        case cfvariant::Number:
            out += varName + " = " + std::to_string(v.m_int) + ";\r\n";
            break;
        case cfvariant::Long:
            out += varName + " = " + std::to_string(v.m_long) + ";\r\n";
            break;
        case cfvariant::Float:
            out += varName + " = " + javaDoubleToString(v.m_double) + ";\r\n";
            break;
        case cfvariant::Boolean:
            out += varName + " = " + (v.m_bool ? "true" : "false") + ";\r\n";
            break;
        case cfvariant::String:
            out += varName + " = \"" + escapeJSString(v.m_str ? v.m_str->constData() : "") + "\";\r\n";
            break;
        case cfvariant::DateTime: {
            struct tm tm = cfml::daysToTm(v.m_double);
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%d, %d, %d, %d, %d, %d",
                          tm.tm_year + 1900, tm.tm_mon, tm.tm_mday,
                          tm.tm_hour, tm.tm_min, tm.tm_sec);
            out += varName + " =  new Date(" + buf + ");\r\n";
            break;
        }
        case cfvariant::Struct:
            renderJsStruct(v, varName, out);
            break;
        case cfvariant::Array:
            if (v.m_queryColOwner) {
                renderJsValue(cfml::queryColumnFirstCell(&v), varName, out);
                break;
            }
            renderJsArray(v, varName, out);
            break;
        case cfvariant::Query:
            renderJsQuery(v, varName, out);
            break;
        case cfvariant::Null:
            out += varName + " = null;\r\n";
            break;
        default:
            out += varName + " = null;\r\n";
            break;
    }
}

std::string renderJsTop(const cfvariant &v, const std::string &varName)
{
    std::string out;
    std::string top = varName.empty() ? "x" : varName;
    renderJsValue(v, top, out);
    return out;
}

} // namespace

namespace cfml {

void cf_wddx_tag(string *out, const cfvariant *attrs,
                 void *cgi, void *server, void *cookie, void *application,
                 void *session, void *url, void *form, void *variables)
{
    std::string rawAction = attrStr(attrs, "action");
    std::string action;
    for (char c : rawAction) action += static_cast<char>(toupper((unsigned char)c));
    const cfvariant *input = attrOf(attrs, "input");

    // Runtime attribute validation (CF validates the TLD attributes against the
    // evaluated action value; the compile-time path already handled static
    // actions, so this runs for dynamically evaluated action values).
    static const std::unordered_set<std::string> validActions = {
        "CFML2JS", "CFML2WDDX", "WDDX2JS", "WDDX2CFML"};
    if (validActions.find(action) == validActions.end()) {
        throw webstrada::exception(webstrada::string("Attribute validation error for CFWDDX."),
            webstrada::string(("The value of the ACTION attribute, which is currently " +
                              rawAction + ", must be one of the values: CFML2JS,CFML2WDDX,WDDX2JS,WDDX2CFML.").c_str()));
    }
    // Per-action valid attribute set.
    std::unordered_set<std::string> validAttrs;
    std::vector<std::string> required;
    if (action == "CFML2WDDX") {
        validAttrs = {"action", "input", "output", "usetimezoneinfo"};
        required = {"input"};
    } else if (action == "WDDX2CFML") {
        validAttrs = {"action", "input", "output", "validate"};
        required = {"input", "output"};
    } else {
        validAttrs = {"action", "input", "output", "toplevelvariable"};
        required = {"input", "toplevelvariable"};
    }
    // TLD-known attributes present but invalid for this action.
    static const char *tldAttrs[] = {"action", "input", "output", "usetimezoneinfo",
                                     "validate", "toplevelvariable"};
    std::vector<std::string> invalid;
    for (const char *a : tldAttrs) {
        if (attrOf(attrs, a) && validAttrs.find(a) == validAttrs.end()) invalid.push_back(a);
    }
    if (!invalid.empty()) {
        std::sort(invalid.begin(), invalid.end());
        std::string list;
        for (size_t i = 0; i < invalid.size(); i++) {
            std::string u = invalid[i];
            for (auto &c : u) c = static_cast<char>(toupper((unsigned char)c));
            if (i) list += ",";
            list += u;
        }
        std::string validList;
        std::vector<std::string> sorted(validAttrs.begin(), validAttrs.end());
        std::sort(sorted.begin(), sorted.end());
        for (size_t i = 0; i < sorted.size(); i++) {
            std::string u = sorted[i];
            for (auto &c : u) c = static_cast<char>(toupper((unsigned char)c));
            if (i) validList += ",";
            validList += u;
        }
        throw webstrada::exception(webstrada::string("Attribute validation error for cfwddx."),
            webstrada::string(("It does not allow the attribute(s) " + list +
                              ". The valid attribute(s) are " + validList + ".").c_str()));
    }
    // Missing required attributes.
    std::vector<std::string> missing;
    for (const auto &r : required) {
        if (!attrOf(attrs, r.c_str())) missing.push_back(r);
    }
    if (!missing.empty()) {
        std::sort(missing.begin(), missing.end());
        std::string list;
        for (size_t i = 0; i < missing.size(); i++) {
            std::string u = missing[i];
            for (auto &c : u) c = static_cast<char>(toupper((unsigned char)c));
            if (i) list += ",";
            list += u;
        }
        throw webstrada::exception(webstrada::string("Attribute validation error for cfwddx."),
            webstrada::string(("When the value of the ACTION attribute is " + action +
                              ", it requires the attribute(s): " + list + ".").c_str()));
    }

    std::string outputName = attrStr(attrs, "output");
    const cfvariant *useTimezoneVal = attrOf(attrs, "usetimezoneinfo");
    bool useTimezone = useTimezoneVal ? cfml::cfmlBoolean(useTimezoneVal, true) : true;

    std::string result;
    if (action == "WDDX2CFML") {
        // Deserialize a WDDX packet to CFML.
        std::string pkt = safe_to_std_string(*input);
        xmlDocPtr doc = xmlReadMemory(pkt.c_str(), static_cast<int>(pkt.size()), "noname.xml", nullptr,
                                      XML_PARSE_NONET | XML_PARSE_NOENT | XML_PARSE_NOCDATA);
        if (!doc) {
            throw webstrada::exception(webstrada::string("Application"),
                webstrada::string("Unable to deserialize the WDDX packet. Invalid XML."));
        }
        xmlNodePtr dataNode = nullptr;
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (root && xmlStrcmp(root->name, BAD_CAST "wddxPacket") == 0) {
            for (xmlNodePtr ch = root->children; ch; ch = ch->next) {
                if (ch->type == XML_ELEMENT_NODE && xmlStrcmp(ch->name, BAD_CAST "data") == 0) {
                    dataNode = ch;
                    break;
                }
            }
        }
        cfvariant res;
        if (dataNode) {
            xmlNodePtr valNode = nullptr;
            for (xmlNodePtr ch = dataNode->children; ch; ch = ch->next) {
                if (ch->type == XML_ELEMENT_NODE) { valNode = ch; break; }
            }
            if (valNode) {
                deserializeWddxElement(valNode, res);
            } else {
                res = cfvariant("");
            }
        }
        xmlFreeDoc(doc);
        if (!outputName.empty()) {
            // CF's WddxTag sets the output attribute to the deserialized value
            // (pageContext.setAttribute), preserving its type.
            cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                             static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                             static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                             static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                             outputName.c_str(), &res);
            return;
        }
        // No output variable (unreachable via the tag — wddx2cfml requires
        // output): the string rendering goes to the page.
        result = safe_to_std_string(res);
    } else if (action == "CFML2WDDX") {
        result = "<wddxPacket version='1.0'><header/><data>";
        serializeWddxValue(*input, result, useTimezone);
        result += "</data></wddxPacket>";
    } else if (action == "CFML2JS") {
        std::string top = attrStr(attrs, "toplevelvariable");
        result = renderJsTop(*input, top);
    } else if (action == "WDDX2JS") {
        std::string pkt = safe_to_std_string(*input);
        xmlDocPtr doc = xmlReadMemory(pkt.c_str(), static_cast<int>(pkt.size()), "noname.xml", nullptr,
                                      XML_PARSE_NONET | XML_PARSE_NOENT | XML_PARSE_NOCDATA);
        if (!doc) {
            throw webstrada::exception(webstrada::string("Application"),
                webstrada::string("Unable to deserialize the WDDX packet. Invalid XML."));
        }
        xmlNodePtr dataNode = nullptr;
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (root && xmlStrcmp(root->name, BAD_CAST "wddxPacket") == 0) {
            for (xmlNodePtr ch = root->children; ch; ch = ch->next) {
                if (ch->type == XML_ELEMENT_NODE && xmlStrcmp(ch->name, BAD_CAST "data") == 0) {
                    dataNode = ch;
                    break;
                }
            }
        }
        cfvariant res;
        if (dataNode) {
            xmlNodePtr valNode = nullptr;
            for (xmlNodePtr ch = dataNode->children; ch; ch = ch->next) {
                if (ch->type == XML_ELEMENT_NODE) { valNode = ch; break; }
            }
            if (valNode) deserializeWddxElement(valNode, res);
        }
        xmlFreeDoc(doc);
        std::string top = attrStr(attrs, "toplevelvariable");
        result = renderJsTop(res, top);
    }

    if (!outputName.empty()) {
        cfvariant res(result.c_str());
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         outputName.c_str(), &res);
    } else if (out) {
        cfwriteoutput(*out, result.c_str(), result.size());
    }
}

} // namespace cfml

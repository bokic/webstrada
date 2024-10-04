/**
 * @file common.cpp
 * @brief Shared helper code for the CFML built-in functions.
 */

#include "common.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <climits>
#include <string>
#include <vector>
#include <set>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <ctime>
#include <map>
#include "../cftags/common.h"
#include <webstrada/cfvariant.h>
#include <webstrada/component.h>
#include <webstrada/string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/xmlschemas.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>
#include <cstddef>
#include <cstdint>
#include <strings.h>
#include <webstrada/upload.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <webstrada/locale.h>
#include <webstrada/scope_store.h>
#include <charconv>
#include <thread>
#include <json-c/json.h>
#include <webstrada/cfimage.h>
#include <cairo.h>
#include <jpeglib.h>
#include <zlib.h>
#include <cfloat>
#include <setjmp.h>
#include <functional>

namespace cfml {

static const char *CFMX_COMPAT_ERROR = "The CFMX_COMPAT algorithm is not supported by the Security Provider you have chosen.";

const char cryptoHexDigits[] = "0123456789ABCDEF";

thread_local const cfml::LocaleInfo *g_currentLocale = nullptr;

thread_local webstrada::string g_currentLocaleStr;

thread_local std::set<const void*> g_serializeVisited;

thread_local bool g_cfdump_style_emitted = false;

thread_local bool g_cfdump_abort_pending = false;

thread_local std::string g_cfdump_style_cache;

thread_local bool g_cfdump_udf_first_space = true;

thread_local std::set<int> g_cfdump_udf_seen_depths;

thread_local bool g_randGenInitialized = false;

const std::string kImageFormats =
    "BMP,GIF,JFIF,JPEG,JPEG 2000,JPEG-LOSSLESS,JPEG-LS,JPEG2000,JPG,PNG,PNM,RAW,TIF,TIFF,WBMP,WEBP";

static const std::string kColorTail =
    "The color attribute recognizes the following colors directly: black, white, gray, mediumgray, darkgray, red, blue, green, pink, orange, magenta, yellow and cyan. You must enter any other color in RGB or hex format.  ";

static const std::string kColorRequired =
    "The passed color argument is not in the required format.\n" + kColorTail;

static const std::string kColorRGB =
    "The passed color value is not in proper RGB format. Ensure that values are in the range of 0-255 and that the values are comma-delimited, for example, \"255,0,0\".  \n" + kColorTail;

static const std::string kColorHex =
    "The passed color value is not in proper hex format, which is either a hex color with # (for example, \"##FF00FF\") or without # (for example, \"FF00FF\").\n" + kColorTail;

// ---- from fn_list ----

string joinList(const std::vector<string> &elements, const string &delim) {
    string d = delim.isEmpty() ? "," : delim.first(1);
    string res;
    for (size_t i = 0; i < elements.size(); i++) {
        if (i > 0) res += d;
        res += elements[i];
    }
    return res;
}

string joinListItems(const std::vector<string> &items, const string &delim)
{
    string out;
    for (size_t i = 0; i < items.size(); i++) {
        if (i > 0) out += delim;
        out += items[i];
    }
    return out;
}

std::vector<string> splitList(const string &listStr, const string &delim, bool includeEmptyFields)
{
    std::vector<string> res;
    if (listStr.isEmpty()) {
        if (includeEmptyFields) res.push_back("");
        return res;
    }
    string actualDelim = delim.isEmpty() ? "," : delim;
    string cur;
    for (int i = 0; i < listStr.length(); i++) {
        char c = listStr.at(i);
        bool isDelim = false;
        for (int j = 0; j < actualDelim.length(); j++) {
            if (c == actualDelim.at(j)) { isDelim = true; break; }
        }
        if (isDelim) {
            if (includeEmptyFields || !cur.trimmed().isEmpty()) res.push_back(cur.trimmed());
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (includeEmptyFields || !cur.trimmed().isEmpty()) res.push_back(cur.trimmed());
    return res;
}

// ---- from fn_struct ----

std::vector<string> structOrderedKeys(const cfvariant &st)
{
    std::vector<string> keys;
    if (st.m_struct) {
        if (st.m_structInsertOrder) {
            for (const auto &k : *st.m_structInsertOrder) keys.push_back(k);
        } else {
            for (const auto &p : *st.m_struct) keys.push_back(p.first);
        }
    }
    return keys;
}

// ---- from fn_string ----

webstrada::string escapeHtmlEdit(const webstrada::string &in) {
    webstrada::string out;
    for (int i = 0; i < in.length(); i++) {
        char c = in.at(i);
        switch (c) {
            case '\r':
                break;
            case '"':
                out.append("&quot;");
                break;
            case '&':
                out.append("&amp;");
                break;
            case '\'':
                out.append("'");
                break;
            case '<':
                out.append("&lt;");
                break;
            case '>':
                out.append("&gt;");
                break;
            default:
                out.append(c);
                break;
        }
    }
    return out;
}

void throwSequenceNotRecognized(char c)
{
    std::string msg = std::string("Sequence (?") + c + "...) not recognized";
    throw webstrada::exception(webstrada::string(msg.c_str()));
}

std::string normalizeRePattern(const std::string &pat)
{
    static const char *literalize = "uUlLFvpPNRXKQEv"; // \x -> bare literal x
    std::string out;
    bool inClass = false;
    size_t i = 0;
    while (i < pat.size()) {
        char c = pat[i];
        if (c == '\\') {
            if (i + 1 >= pat.size()) { out += c; i++; continue; }
            char n = pat[i + 1];
            if (n == '\\') { out += "\\\\"; i += 2; continue; }
            if (strchr(literalize, n)) { out += n; i += 2; continue; }
            out += c;
            out += n;
            i += 2;
            continue;
        }
        if (!inClass && c == '[') { inClass = true; out += c; i++; continue; }
        if (inClass && c == ']') { inClass = false; out += c; i++; continue; }
        if (!inClass && c == '(' && i + 1 < pat.size() && pat[i + 1] == '?') {
            char d = (i + 2 < pat.size()) ? pat[i + 2] : '\0';
            if (d == '<' || d == '\'' || d == 'P') {
                throwSequenceNotRecognized(d);
            }
            if (d == '>') {
                throwSequenceNotRecognized('>');
            }
            if (d != '\0' && (isalpha(static_cast<unsigned char>(d)) || d == '-')) {
                // Scoped inline modifier (?imsx-: ...) -> ORO rejects. A bare
                // inline flag (?i) ends in ')' and is accepted by ORO.
                size_t j = i + 2;
                while (j < pat.size() && (isalpha(static_cast<unsigned char>(pat[j])) || pat[j] == '-')) j++;
                if (j < pat.size() && pat[j] == ':') {
                    throwSequenceNotRecognized(':');
                }
            }
        }
        out += c;
        i++;
    }
    return out;
}

pcre2_code *reCompile(const std::string &pattern, bool nocase)
{
    uint32_t options = PCRE2_DOTALL; // CF compiles ORO patterns with SINGLELINE
    if (nocase) options |= PCRE2_CASELESS;
    std::string normalized = normalizeRePattern(pattern);
    int errcode = 0;
    PCRE2_SIZE erroffset = 0;
    pcre2_code *code = pcre2_compile(
        reinterpret_cast<PCRE2_SPTR>(normalized.c_str()), normalized.size(),
        options, &errcode, &erroffset, nullptr);
    if (!code) {
        PCRE2_UCHAR msg[256];
        pcre2_get_error_message(errcode, msg, sizeof(msg));
        throw webstrada::exception(
            webstrada::string("Malformed regular expression: ") + webstrada::string(reinterpret_cast<const char *>(msg)));
    }
    return code;
}

bool reFindNext(pcre2_code *code, const std::string &subject,
                PCRE2_SIZE startOffset, ReMatchResult &out)
{
    if (startOffset > subject.size()) return false;
    pcre2_match_data *md = pcre2_match_data_create_from_pattern(code, nullptr);
    int rc = pcre2_match(code, reinterpret_cast<PCRE2_SPTR>(subject.c_str()),
                         subject.size(), startOffset, 0, md, nullptr);
    if (rc == PCRE2_ERROR_NOMATCH) {
        pcre2_match_data_free(md);
        return false;
    }
    if (rc < 0) {
        PCRE2_UCHAR msg[256];
        pcre2_get_error_message(rc, msg, sizeof(msg));
        pcre2_match_data_free(md);
        throw webstrada::exception(
            webstrada::string("Regular expression match error: ") + webstrada::string(reinterpret_cast<const char *>(msg)));
    }
    PCRE2_SIZE *ovec = pcre2_get_ovector_pointer(md);
    uint32_t groupCount = 0;
    pcre2_pattern_info(code, PCRE2_INFO_CAPTURECOUNT, &groupCount);
    out.start = ovec[0];
    out.end = ovec[1];
    out.groups.resize(groupCount + 1);
    for (uint32_t g = 0; g <= groupCount; g++) {
        if (ovec[2 * g] == PCRE2_UNSET) out.groups[g] = {PCRE2_UNSET, PCRE2_UNSET};
        else out.groups[g] = {ovec[2 * g], ovec[2 * g + 1]};
    }
    pcre2_match_data_free(md);
    return true;
}

// ---- CFML type-validation checks (shared by isvalid() and <cfparam>) ----
// Moved here from fn_isvalid.cpp; see common.h for the rationale.

// Strict Java Double.parseDouble-like numeric check: optional sign, digits,
// optional '.', optional exponent. Comma/currency/whitespace-inner rejected.
bool cfmlStrictParseDouble(const std::string &s, double &out) {
    const char *p = s.c_str();
    while (*p && std::isspace(static_cast<unsigned char>(*p))) p++;
    if (!*p) return false;
    bool neg = false;
    if (*p == '+' || *p == '-') { neg = (*p == '-'); p++; }
    if (*p == 'N' && std::strncmp(p, "NaN", 3) == 0) { out = std::nan(""); return true; }
    if (std::strncmp(p, "Infinity", 8) == 0) { out = neg ? -INFINITY : INFINITY; return true; }
    bool anyDigit = false;
    while (std::isdigit(static_cast<unsigned char>(*p))) { anyDigit = true; p++; }
    if (*p == '.') {
        p++;
        while (std::isdigit(static_cast<unsigned char>(*p))) { anyDigit = true; p++; }
    }
    if (!anyDigit) return false;
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        bool expDigit = false;
        while (std::isdigit(static_cast<unsigned char>(*p))) { expDigit = true; p++; }
        if (!expDigit) return false;
    }
    while (*p && std::isspace(static_cast<unsigned char>(*p))) p++;
    if (*p) return false;
    out = std::strtod(s.c_str(), nullptr);
    return true;
}

// Java Integer.parseInt-like: strict integer, optional sign, digits only.
bool cfmlStrictParseInt(const std::string &s, long long &out) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[i] == '+' || s[i] == '-') i++;
    if (i >= s.size()) return false;
    long long v = 0;
    for (; i < s.size(); i++) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        v = v * 10 + (s[i] - '0');
    }
    out = (s[0] == '-') ? -v : v;
    return true;
}

bool cfmlRegexFullMatch(const std::string &pattern, const std::string &subject) {
    int errcode = 0;
    PCRE2_SIZE erroffset = 0;
    pcre2_code *code = pcre2_compile(
        reinterpret_cast<PCRE2_SPTR>(pattern.c_str()), pattern.size(),
        PCRE2_DOTALL, &errcode, &erroffset, nullptr);
    if (!code) return false;
    pcre2_match_data *md = pcre2_match_data_create_from_pattern(code, nullptr);
    int rc = pcre2_match(code, reinterpret_cast<PCRE2_SPTR>(subject.c_str()),
                         subject.size(), 0, 0, md, nullptr);
    // CF's validators use ORO Perl5 `matches()`, which requires the pattern to
    // consume the ENTIRE subject (unlike a plain pcre2 substring find). For an
    // unanchored pattern like `[A-Fa-f0-9]{8,8}-...` this rejects a match that
    // appears only as a substring.
    bool full = false;
    if (rc >= 0) {
        PCRE2_SIZE *ovec = pcre2_get_ovector_pointer(md);
        full = (ovec[0] == 0 && ovec[1] == subject.size());
    }
    pcre2_match_data_free(md);
    pcre2_code_free(code);
    return full;
}

bool cfmlIsUsDate(const std::string &str) {
    std::string s = str;
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    s = s.substr(b, e - b);
    if (s.empty()) return false;

    int isplit = (int)s.find('/');
    std::string splitchr = "/";
    if (isplit == -1) { isplit = (int)s.find('.'); splitchr = "."; }
    if (isplit == -1) { isplit = (int)s.find('-'); splitchr = "-"; }
    if (isplit == -1 || isplit == (int)s.size()) return false;
    std::string sMonth = s.substr(0, isplit);
    if (sMonth.empty()) return false;
    int isplit2 = (int)s.find(splitchr, isplit + 1);
    if (isplit2 == -1 || isplit2 + 1 == (int)s.size()) return false;
    std::string sDay = s.substr(sMonth.size() + 1, isplit2 - (int)sMonth.size() - 1);
    if (sDay.empty()) return false;
    std::string sYear = s.substr(isplit2 + 1);

    long long month, day, year;
    if (!cfmlStrictParseInt(sMonth, month)) return false;
    if (month < 1 || month > 12) return false;
    if (!cfmlStrictParseInt(sYear, year)) return false;
    if (sYear.size() != 1 && sYear.size() != 2 && sYear.size() != 4) return false;
    if (year < 0 || year > 9999) return false;
    if (!cfmlStrictParseInt(sDay, day)) return false;
    int maxDay = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) maxDay = 30;
    else if (month == 2) {
        if (year % 4 > 0) maxDay = 28;
        else if (year % 100 == 0 && year % 400 > 0) maxDay = 28;
        else maxDay = 29;
    }
    if (day < 1 || day > maxDay) return false;
    return true;
}

bool cfmlIsEuroDate(const std::string &str) {
    std::string s = str;
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    s = s.substr(b, e - b);
    if (s.empty()) return false;
    if (s[0] == '{' && s.back() == '}') {
        double dummy = 0;
        return parseDateTimeStr(webstrada::string(s.c_str()), dummy);
    }
    int sep = (int)s.find_first_of("/.-");
    if (sep <= 0 || sep == (int)s.size()) return false;
    int sep2 = (int)s.find(s[sep], sep + 1);
    if (sep2 == -1 || sep2 + 1 == (int)s.size()) return false;
    std::string sDay = s.substr(0, sep);
    std::string sMonth = s.substr(sep + 1, sep2 - sep - 1);
    std::string sYear = s.substr(sep2 + 1);
    long long day, month, year;
    if (!cfmlStrictParseInt(sDay, day)) return false;
    if (!cfmlStrictParseInt(sMonth, month)) return false;
    if (!cfmlStrictParseInt(sYear, year)) return false;
    if (month < 1 || month > 12) return false;
    int maxDay = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) maxDay = 30;
    else if (month == 2) {
        if (year % 4 > 0) maxDay = 28;
        else if (year % 100 == 0 && year % 400 > 0) maxDay = 28;
        else maxDay = 29;
    }
    if (day < 1 || day > maxDay) return false;
    return true;
}

bool cfmlIsTimeString(const std::string &str) {
    std::string s = str;
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    s = s.substr(b, e - b);
    if (s.empty()) return false;
    int h = 0, m = 0, sec = 0;
    std::string lower = s;
    for (auto &c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    bool ampm = lower.find("am") != std::string::npos || lower.find("pm") != std::string::npos;
    std::string t = s;
    if (ampm) {
        size_t p = t.find_first_of("apAP");
        if (p != std::string::npos) t = t.substr(0, p);
        while (!t.empty() && std::isspace(static_cast<unsigned char>(t.back()))) t.pop_back();
    }
    if (std::sscanf(t.c_str(), "%d:%d:%d", &h, &m, &sec) >= 2) {
        if (h >= 0 && h <= 23 && m >= 0 && m <= 59 && sec >= 0 && sec <= 59) return true;
        return false;
    }
    return false;
}

bool cfmlLuhnCheck(const std::string &digits) {
    int checkdigit = 0;
    bool doubledigit = (digits.size() % 2 != 1);
    for (char c : digits) {
        int tempdigit = c - '0';
        if (doubledigit) {
            int tempdigit2 = tempdigit * 2;
            checkdigit += tempdigit2 % 10;
            if (tempdigit2 / 10 >= 1) checkdigit++;
            doubledigit = false;
        } else {
            checkdigit += tempdigit;
            doubledigit = true;
        }
    }
    return checkdigit % 10 == 0;
}

bool cfmlIsValidVariableName(const std::string &id) {
    if (!id.empty() && id.back() == '.') return false;
    if (!id.empty() && (std::isdigit(static_cast<unsigned char>(id[0])) || id[0] == '.')) return false;
    size_t pos = 0;
    while (pos < id.size()) {
        size_t segStart = pos;
        while (pos < id.size() && id[pos] != '.') pos++;
        std::string seg = id.substr(segStart, pos - segStart);
        if (seg.empty()) return false;
        char c0 = seg[0];
        if (!(std::isalpha(static_cast<unsigned char>(c0)) || c0 == '_' || c0 == '$')) return false;
        for (size_t i = 1; i < seg.size(); i++) {
            char c = seg[i];
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
        }
        if (pos < id.size() && id[pos] == '.') pos++;
    }
    return true;
}

bool cfmlIsValidNumeric(const cfvariant *value) {
    if (!value) return false;
    if (value->m_type == cfvariant::Boolean) {
        try {
            getDoubleValue(*const_cast<cfvariant*>(value));
            return true;
        } catch (...) {
            return false;
        }
    }
    if (value->m_type == cfvariant::Number || value->m_type == cfvariant::Float ||
        value->m_type == cfvariant::Long || value->m_type == cfvariant::DateTime) {
        return true;
    }
    std::string s = const_cast<cfvariant*>(value)->toString().constData();
    std::string l = s;
    for (auto &c : l) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    size_t tb = 0, te = l.size();
    while (tb < te && std::isspace(static_cast<unsigned char>(l[tb]))) tb++;
    while (te > tb && std::isspace(static_cast<unsigned char>(l[te - 1]))) te--;
    std::string lt = l.substr(tb, te - tb);
    if (lt == "true" || lt == "false" || lt == "yes" || lt == "no") return false;
    double v = 0;
    return cfmlStrictParseDouble(s, v);
}

bool cfmlIsValidInteger(const cfvariant *value) {
    if (!value) return false;
    if (value->m_type == cfvariant::Number || value->m_type == cfvariant::Long) {
        return true;
    }
    if (value->m_type == cfvariant::Float) {
        double d = value->m_double;
        return d == static_cast<double>(static_cast<long long>(d));
    }
    std::string s = const_cast<cfvariant*>(value)->toString().constData();
    long long v = 0;
    return cfmlStrictParseInt(s, v);
}

bool cfmlIsValidUrl(const std::string &urlString) {
    if (urlString.find(':') == std::string::npos) return false;
    if (urlString.rfind("http://", 0) == 0 || urlString.rfind("https://", 0) == 0 ||
        urlString.rfind("ftp://", 0) == 0 || urlString.rfind("file://", 0) == 0 ||
        urlString.rfind("news:", 0) == 0) {
        std::string proto = urlString.substr(0, urlString.find(':'));
        if (proto == "http" || proto == "https" || proto == "ftp" || proto == "file") {
            if (urlString.find("://") == std::string::npos) return false;
            std::string rest = urlString.substr(urlString.find("://") + 3);
            std::string host = rest;
            size_t slash = rest.find('/');
            if (slash != std::string::npos) host = rest.substr(0, slash);
            size_t at = host.find('@');
            if (at != std::string::npos) host = host.substr(at + 1);
            if (host.empty()) return false;
        }
        return true;
    }
    return false;
}

webstrada::cfvariant *makeReFindStruct(const ReMatchResult &m, const std::string &subject)
{
    auto *ret = new webstrada::cfvariant(webstrada::cfvariant::Struct);
    webstrada::cfvariant matchArr(webstrada::cfvariant::Array);
    webstrada::cfvariant posArr(webstrada::cfvariant::Array);
    webstrada::cfvariant lenArr(webstrada::cfvariant::Array);
    for (size_t g = 0; g < m.groups.size(); g++) {
        PCRE2_SIZE s = m.groups[g].first;
        PCRE2_SIZE e = m.groups[g].second;
        if (s == PCRE2_UNSET) {
            matchArr.insert(webstrada::cfvariant(""));
            posArr.insert(webstrada::cfvariant(0));
            lenArr.insert(webstrada::cfvariant(0));
        } else {
            matchArr.insert(webstrada::cfvariant(std::string(subject, s, e - s).c_str()));
            posArr.insert(webstrada::cfvariant(static_cast<int>(s + 1)));
            lenArr.insert(webstrada::cfvariant(static_cast<int>(e - s)));
        }
    }
    ret->structSet("MATCH", matchArr);
    ret->structSet("POS", posArr);
    ret->structSet("LEN", lenArr);
    return ret;
}

webstrada::cfvariant *makeReFindEmptyStruct()
{
    auto *ret = new webstrada::cfvariant(webstrada::cfvariant::Struct);
    webstrada::cfvariant matchArr(webstrada::cfvariant::Array);
    matchArr.insert(webstrada::cfvariant(""));
    webstrada::cfvariant posArr(webstrada::cfvariant::Array);
    posArr.insert(webstrada::cfvariant(0));
    webstrada::cfvariant lenArr(webstrada::cfvariant::Array);
    lenArr.insert(webstrada::cfvariant(0));
    ret->structSet("MATCH", matchArr);
    ret->structSet("POS", posArr);
    ret->structSet("LEN", lenArr);
    return ret;
}

webstrada::cfvariant *doReFind(const webstrada::string &reVal, const webstrada::string &strVal,
                            int start, bool returnsub, const webstrada::string &scopeVal, bool nocase)
{
    std::string re = reVal.constData() ? reVal.constData() : "";
    std::string subject = strVal.constData() ? strVal.constData() : "";
    if (start < 1) start = 1;

    std::string scope = scopeVal.constData() ? scopeVal.constData() : "";
    bool all = scopeVal.compareCaseInsensitive("all") == 0;
    bool one = scopeVal.compareCaseInsensitive("one") == 0;
    if (!all && !one) {
        throw webstrada::exception(webstrada::string("The scope argument of the ") +
                                  (nocase ? "REFindNoCase" : "REFind") +
                                  " function has an invalid value " + scopeVal + ".",
                                  "Allowed values are ONE, ALL.");
    }

    if (start > (int)subject.size()) {
        if (returnsub) {
            if (one) return makeReFindEmptyStruct();
            auto *arr = new webstrada::cfvariant(webstrada::cfvariant::Array);
            arr->insert(*makeReFindEmptyStruct());
            return arr;
        }
        auto *zero = new webstrada::cfvariant(0);
        return zero;
    }

    pcre2_code *code = reCompile(re, nocase);
    PCRE2_SIZE offset = static_cast<PCRE2_SIZE>(start - 1);
    ReMatchResult m;
    bool found = reFindNext(code, subject, offset, m);
    pcre2_code_free(code);
    if (!found) {
        if (returnsub) {
            if (one) return makeReFindEmptyStruct();
            auto *arr = new webstrada::cfvariant(webstrada::cfvariant::Array);
            cfvariant *es = makeReFindEmptyStruct();
            cf_register_temp(es);
            arr->insert(*es);
            return arr;
        }
        auto *zero = new webstrada::cfvariant(0);
        return zero;
    }

    if (one) {
        if (returnsub) return makeReFindStruct(m, subject);
        auto *pos = new webstrada::cfvariant(static_cast<int>(m.start + 1));
        return pos;
    }

    auto *arr = new webstrada::cfvariant(webstrada::cfvariant::Array);
    if (returnsub) {
        cfvariant *rs = makeReFindStruct(m, subject);
        cf_register_temp(rs);
        arr->insert(*rs);
    }
    else arr->insert(webstrada::cfvariant(static_cast<int>(m.start + 1)));
    PCRE2_SIZE next = (m.end > m.start) ? m.end : m.start + 1;
    code = reCompile(re, nocase);
    while (next <= subject.size() && reFindNext(code, subject, next, m)) {
        if (returnsub) {
            cfvariant *rs = makeReFindStruct(m, subject);
            cf_register_temp(rs);
            arr->insert(*rs);
        }
        else arr->insert(webstrada::cfvariant(static_cast<int>(m.start + 1)));
        next = (m.end > m.start) ? m.end : m.start + 1;
    }
    pcre2_code_free(code);
    return arr;
}

webstrada::cfvariant *doReMatch(const webstrada::string &reVal, const webstrada::string &strVal, bool nocase)
{
    std::string re = reVal.constData() ? reVal.constData() : "";
    std::string subject = strVal.constData() ? strVal.constData() : "";
    pcre2_code *code = reCompile(re, nocase);
    auto *arr = new webstrada::cfvariant(webstrada::cfvariant::Array);
    PCRE2_SIZE offset = 0;
    ReMatchResult m;
    while (reFindNext(code, subject, offset, m)) {
        arr->insert(webstrada::cfvariant(std::string(subject, m.start, m.end - m.start).c_str()));
        offset = (m.end > m.start) ? m.end : m.start + 1;
    }
    pcre2_code_free(code);
    return arr;
}

std::string substitutionPass(const std::string &subject, const std::string &pattern,
                             char appendLiteral)
{
    pcre2_code *code = reCompile(pattern, false);
    std::string result;
    PCRE2_SIZE offset = 0;
    ReMatchResult m;
    while (reFindNext(code, subject, offset, m)) {
        result.append(subject, offset, m.start - offset);
        if (m.groups[1].first != PCRE2_UNSET) {
            result.append(subject, m.groups[1].first, m.groups[1].second - m.groups[1].first);
        }
        result += appendLiteral;
        offset = (m.end > m.start) ? m.end : m.start + 1;
    }
    result.append(subject, offset, std::string::npos);
    pcre2_code_free(code);
    return result;
}

std::string preprocessReplacement(const std::string &subst)
{
    // escPattern: (\A|.)(?=\$|\\\\(?!\\\\*[\dUuLlE]))
    static const char *escPattern =
        "(\\A|.)(?=\\$|\\\\(?!\\\\*[\\dUuLlE]))";
    // backrefPattern: (\A|[^\\]|(\A|[^\\])\\)\\(?=\d+)
    static const char *backrefPattern =
        "(\\A|[^\\\\]|(\\A|[^\\\\])\\\\\\\\)\\\\(?=\\d+)";
    std::string s = substitutionPass(subst, escPattern, '\\');
    s = substitutionPass(s, backrefPattern, '$');
    return s;
}

std::string renderSubstitution(const std::string &subst, const std::string &subject,
                               const ReMatchResult &m)
{
    // Token list: literal text, group reference, or case control.
    enum { T_LIT, T_GROUP, T_UC_FIRST, T_LC_FIRST, T_UC_MODE, T_LC_MODE, T_END_MODE };
    struct Tok { int kind; std::string text; int group; };
    std::vector<Tok> toks;
    size_t i = 0;
    const size_t n = subst.size();
    while (i < n) {
        char c = subst[i];
        if (c == '\\') {
            if (i + 1 >= n) { toks.push_back({T_LIT, "\\", 0}); i++; continue; }
            char d = subst[i + 1];
            if (d == '\\') { toks.push_back({T_LIT, "\\", 0}); i += 2; }
            else if (d == '$') { toks.push_back({T_LIT, "$", 0}); i += 2; }
            else if (d == 'u') { toks.push_back({T_UC_FIRST, "", 0}); i += 2; }
            else if (d == 'l') { toks.push_back({T_LC_FIRST, "", 0}); i += 2; }
            else if (d == 'U') { toks.push_back({T_UC_MODE, "", 0}); i += 2; }
            else if (d == 'L') { toks.push_back({T_LC_MODE, "", 0}); i += 2; }
            else if (d == 'E') { toks.push_back({T_END_MODE, "", 0}); i += 2; }
            else { toks.push_back({T_LIT, std::string("\\") + d, 0}); i += 2; }
        } else if (c == '$') {
            if (i + 1 < n && isdigit(static_cast<unsigned char>(subst[i + 1]))) {
                size_t j = i + 1;
                int g = 0;
                while (j < n && isdigit(static_cast<unsigned char>(subst[j]))) {
                    g = g * 10 + (subst[j] - '0');
                    if (g > 10000) { g = 10000; break; }
                    j++;
                }
                toks.push_back({T_GROUP, "", g});
                i = j;
            } else {
                toks.push_back({T_LIT, "$", 0});
                i++;
            }
        } else {
            std::string lit;
            while (i < n && subst[i] != '\\' && subst[i] != '$') { lit += subst[i]; i++; }
            toks.push_back({T_LIT, lit, 0});
        }
    }

    auto firstCharCase = [](std::string s, bool upper) {
        if (s.empty()) return s;
        unsigned char c = static_cast<unsigned char>(s[0]);
        s[0] = upper ? static_cast<char>(toupper(c)) : static_cast<char>(tolower(c));
        return s;
    };
    auto wholeCase = [](std::string s, bool upper) {
        for (auto &ch : s) ch = upper ? static_cast<char>(toupper(static_cast<unsigned char>(ch)))
                                      : static_cast<char>(tolower(static_cast<unsigned char>(ch)));
        return s;
    };

    std::string out;
    int mode = 0;         // 0 none, 1 uppercase mode, 2 lowercase mode
    int pendingFirst = 0; // 0 none, 1 uppercase first char, 2 lowercase first char
    for (const auto &t : toks) {
        if (t.kind == T_UC_MODE) { mode = 1; continue; }
        if (t.kind == T_LC_MODE) { mode = 2; continue; }
        if (t.kind == T_END_MODE) { mode = 0; continue; }
        if (t.kind == T_UC_FIRST) { pendingFirst = 1; continue; }
        if (t.kind == T_LC_FIRST) { pendingFirst = 2; continue; }
        std::string text;
        if (t.kind == T_GROUP) {
            if (t.group >= 0 && t.group < (int)m.groups.size() && m.groups[t.group].first != PCRE2_UNSET) {
                text.assign(subject, m.groups[t.group].first,
                            m.groups[t.group].second - m.groups[t.group].first);
            }
        } else {
            text = t.text;
        }
        if (mode == 1) text = wholeCase(text, true);
        else if (mode == 2) text = wholeCase(text, false);
        if (pendingFirst) {
            text = firstCharCase(text, pendingFirst == 1);
            pendingFirst = 0;
        }
        out += text;
    }
    return out;
}

webstrada::cfvariant *doReReplace(const webstrada::string &strVal, const webstrada::string &reVal,
                               const webstrada::string &subVal, const webstrada::string &scopeVal, bool nocase)
{
    std::string re = reVal.constData() ? reVal.constData() : "";
    std::string subject = strVal.constData() ? strVal.constData() : "";
    std::string sub = subVal.constData() ? subVal.constData() : "";

    if (re.empty()) {
        throw webstrada::exception("Argument 2 of function REReplace cannot be an empty value.");
    }
    bool all = scopeVal.compareCaseInsensitive("all") == 0;
    bool one = scopeVal.compareCaseInsensitive("one") == 0;
    if (!all && !one) {
        // CF hardcodes "REReplace" here even for REReplaceNoCase (see
        // StringFunc._REReplace).
        throw webstrada::exception(webstrada::string("The scope argument of the REReplace function has an invalid value ") +
                                  scopeVal + ".",
                                  "Allowed values are ONE, ALL.");
    }

    std::string subst = preprocessReplacement(sub);

    pcre2_code *code = reCompile(re, nocase);
    std::string result;
    PCRE2_SIZE offset = 0;
    ReMatchResult m;
    bool first = true;
    while (reFindNext(code, subject, offset, m)) {
        result.append(subject, offset, m.start - offset);
        result += renderSubstitution(subst, subject, m);
        offset = (m.end > m.start) ? m.end : m.start + 1;
        if (one && first) { first = false; break; }
    }
    result.append(subject, offset, std::string::npos);
    pcre2_code_free(code);

    auto *ret = new webstrada::cfvariant(result.c_str());
    return ret;
}

// ---- from fn_date ----

double tmToDays(const struct tm &tm) {
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon + 1;
    int day = tm.tm_mday;

    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;

    long jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
    double days = jdn - 2415019;

    days += (tm.tm_hour + (tm.tm_min + tm.tm_sec / 60.0) / 60.0) / 24.0;
    return days;
}

struct tm daysToTm(double days) {
    struct tm tm_result;
    memset(&tm_result, 0, sizeof(tm_result));

    double int_part;
    double frac_part = std::modf(days, &int_part);
    if (frac_part < 0.0) {
        frac_part += 1.0;
        int_part -= 1.0;
    }

    long jdn = static_cast<long>(int_part) + 2415019;

    long l = jdn + 68569;
    long n = (4 * l) / 146097;
    l = l - (146097 * n + 3) / 4;
    long i = (4000 * (l + 1)) / 1461001;
    l = l - (1461 * i) / 4 + 31;
    long j = (80 * l) / 2447;
    int mday = l - (2447 * j) / 80;
    l = j / 11;
    int mon = j + 2 - 12 * l - 1; // 0-based month
    int year = 100 * (n - 49) + i + l - 1900;

    tm_result.tm_year = year;
    tm_result.tm_mon = mon;
    tm_result.tm_mday = mday;

    int days_in_months[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int actual_year = year + 1900;
    if (actual_year % 4 == 0 && (actual_year % 100 != 0 || actual_year % 400 == 0)) {
        days_in_months[1] = 29;
    }
    int yday = 0;
    for (int m = 0; m < mon; m++) {
        yday += days_in_months[m];
    }
    yday += mday - 1;
    tm_result.tm_yday = yday;

    double total_seconds = frac_part * 86400.0 + 0.5; // with rounding
    int hour = static_cast<int>(total_seconds) / 3600;
    int min = (static_cast<int>(total_seconds) % 3600) / 60;
    int sec = static_cast<int>(total_seconds) % 60;

    tm_result.tm_hour = hour;
    tm_result.tm_min = min;
    tm_result.tm_sec = sec;

    long days_int = static_cast<long>(int_part);
    tm_result.tm_wday = (6 + days_int % 7 + 7) % 7;

    return tm_result;
}

webstrada::string formatDateTime(double days, const webstrada::string &mask, FormatMode mode,
                               const cfml::LocaleInfo *locale) {
    if (!locale) locale = cfml::locale_default();
    struct tm tm = daysToTm(days);

    int year = tm.tm_year + 1900;
    int mon = tm.tm_mon + 1;
    int mday = tm.tm_mday;
    int hour = tm.tm_hour;
    int min = tm.tm_min;
    int sec = tm.tm_sec;
    int wday = tm.tm_wday; // 0 = Sunday

    const char* const *month_names = locale->months;
    const char* const *month_names_short = locale->monthsShort;
    const char* const *day_names = locale->days;
    const char* const *day_names_short = locale->daysShort;

    std::string res;
    webstrada::string trimmedMask = mask.trimmed();
    webstrada::string maskLower = trimmedMask;
    maskLower.toLower();

    // Named masks (short/medium/long/full) resolve to the locale's pattern.
    bool namedMask = false;
    if (maskLower.equals("short")) { namedMask = true; res = mode == ModeTime ? locale->timeShort : mode == ModeDate ? locale->dateShort : (std::string(locale->dateShort) + " " + locale->timeShort); }
    else if (maskLower.equals("medium")) { namedMask = true; res = mode == ModeTime ? locale->timeMedium : mode == ModeDate ? locale->dateMedium : (std::string(locale->dateMedium) + " " + locale->timeMedium); }
    else if (maskLower.equals("long")) { namedMask = true; res = mode == ModeTime ? locale->timeMedium : mode == ModeDate ? locale->dateLong : (std::string(locale->dateLong) + " " + locale->timeMedium); }
    else if (maskLower.equals("full")) { namedMask = true; res = mode == ModeTime ? locale->timeMedium : mode == ModeDate ? locale->dateFull : (std::string(locale->dateFull) + " " + locale->timeMedium); }

    if (!namedMask) {
        if (!trimmedMask.isEmpty()) {
            const char* cstr = trimmedMask.constData();
            if (cstr) res = cstr;
        }
        if (res.empty()) {
            if (mode == ModeDate) res = "dd-mmm-yy";
            else if (mode == ModeTime) res = "hh:nn tt";
            else res = "dd-mmm-yyyy HH:nn:ss";
        }
    }

    // Protect single-quoted literal segments ('de', 'den', ...) so their
    // letters are not mistaken for format tokens (Java/CF SimpleDateFormat
    // quoting). The placeholders use only digits/underscores so no token
    // replacement can touch them; they are restored after Stage 3.
    std::map<std::string, std::string> quotedLiterals;
    {
        std::string protected_res;
        bool inQ = false;
        std::string cur;
        int litIdx = 0;
        for (char c : res) {
            if (c == '\'') {
                if (inQ) {
                    std::string ph = "___" + std::to_string(litIdx++) + "___";
                    quotedLiterals[ph] = cur;
                    protected_res += ph;
                    cur.clear();
                    inQ = false;
                } else {
                    inQ = true;
                }
            } else if (inQ) {
                cur += c;
            } else {
                protected_res += c;
            }
        }
        if (inQ) protected_res += cur; // unterminated quote: keep as literal
        res = protected_res;
    }

    // In ModeTime, treat 'mm'/'m' as 'nn'/'n' (minutes) to avoid colliding with month
    if (mode == ModeTime) {
        auto replaceAllChars = [](std::string &s, const std::string &search, const std::string &replace) {
            size_t pos = 0;
            while ((pos = s.find(search, pos)) != std::string::npos) {
                s.replace(pos, search.length(), replace);
                pos += replace.length();
            }
        };
        replaceAllChars(res, "mm", "nn");
        replaceAllChars(res, "MM", "nn");
        replaceAllChars(res, "m", "n");
        replaceAllChars(res, "M", "n");
    }

    auto replaceAll = [](std::string &s, const std::string &search, const std::string &replace) {
        size_t pos = 0;
        while ((pos = s.find(search, pos)) != std::string::npos) {
            s.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    };

    // Stage 1: Replace formatting tokens with safe, non-colliding placeholder tags
    // Replace in descending order of length to avoid prefix collision

    // Length 4
    replaceAll(res, "yyyy", "_#A#_");
    replaceAll(res, "YYYY", "_#A#_");
    replaceAll(res, "mmmm", "_#C#_");
    replaceAll(res, "MMMM", "_#C#_");
    replaceAll(res, "dddd", "_#X#_");
    replaceAll(res, "DDDD", "_#X#_");

    // Length 3
    replaceAll(res, "mmm", "_#E#_");
    replaceAll(res, "MMM", "_#E#_");
    replaceAll(res, "ddd", "_#S#_");
    replaceAll(res, "DDD", "_#S#_");

    // Length 2
    replaceAll(res, "yy", "_#B#_");
    replaceAll(res, "YY", "_#B#_");
    replaceAll(res, "mm", "_#F#_");
    replaceAll(res, "MM", "_#F#_");
    replaceAll(res, "dd", "_#I#_");
    replaceAll(res, "DD", "_#I#_");
    replaceAll(res, "hh", "_#K#_");
    replaceAll(res, "HH", "_#O#_");
    replaceAll(res, "nn", "_#Q#_");
    replaceAll(res, "ss", "_#U#_");
    replaceAll(res, "tt", "_#W#_");
    replaceAll(res, "TT", "_#W#_");

    // Length 1
    replaceAll(res, "m", "_#G#_");
    replaceAll(res, "M", "_#G#_");
    replaceAll(res, "d", "_#J#_");
    replaceAll(res, "D", "_#J#_");
    replaceAll(res, "h", "_#L#_");
    replaceAll(res, "H", "_#P#_");
    replaceAll(res, "n", "_#R#_");
    replaceAll(res, "s", "_#V#_");
    replaceAll(res, "t", "_#Z#_");
    replaceAll(res, "T", "_#Z#_");

    // Stage 2: Format the values into string buffers
    char yr4Buf[16], yr2Buf[16];
    std::snprintf(yr4Buf, sizeof(yr4Buf), "%04d", year);
    std::snprintf(yr2Buf, sizeof(yr2Buf), "%02d", year % 100);

    char mon2Buf[16], mon1Buf[16];
    std::snprintf(mon2Buf, sizeof(mon2Buf), "%02d", mon);
    std::snprintf(mon1Buf, sizeof(mon1Buf), "%d", mon);

    char day2Buf[16], day1Buf[16];
    std::snprintf(day2Buf, sizeof(day2Buf), "%02d", mday);
    std::snprintf(day1Buf, sizeof(day1Buf), "%d", mday);

    int hour12 = hour % 12;
    if (hour12 == 0) hour12 = 12;

    char hr12_2[16], hr12_1[16];
    std::snprintf(hr12_2, sizeof(hr12_2), "%02d", hour12);
    std::snprintf(hr12_1, sizeof(hr12_1), "%d", hour12);

    char hr24_2[16], hr24_1[16];
    std::snprintf(hr24_2, sizeof(hr24_2), "%02d", hour);
    std::snprintf(hr24_1, sizeof(hr24_1), "%d", hour);

    char min2Buf[16], min1Buf[16];
    std::snprintf(min2Buf, sizeof(min2Buf), "%02d", min);
    std::snprintf(min1Buf, sizeof(min1Buf), "%d", min);

    char sec2Buf[16], sec1Buf[16];
    std::snprintf(sec2Buf, sizeof(sec2Buf), "%02d", sec);
    std::snprintf(sec1Buf, sizeof(sec1Buf), "%d", sec);

    std::string ap2Str = (hour >= 12) ? locale->pm : locale->am;
    std::string ap1Str = (hour >= 12) ? "P" : "A";

    // Stage 3: Replace safe placeholder tags with the formatted values
    replaceAll(res, "_#A#_", yr4Buf);
    replaceAll(res, "_#B#_", yr2Buf);
    {
        std::string monthName = month_names[mon - 1];
        // CF 2025: a BARE full-month mask ("mmmm"/"MMMM" alone) renders the
        // locale's *standalone* month form. For Italian that form is capitalized
        // ("Maggio") while the format form used inside a longer mask is lowercase
        // ("maggio") — verified on the RDS host. Other locales' standalone forms
        // equal their format form, so nothing changes for them.
        std::string trimmedLower = trimmedMask.constData() ? trimmedMask.constData() : "";
        for (auto &c : trimmedLower) c = (char)tolower((unsigned char)c);
        bool bareMonthMask = trimmedLower == "mmmm";
        if (bareMonthMask && locale->standaloneMonthCapitalized && !monthName.empty()) {
            unsigned char f0 = static_cast<unsigned char>(monthName[0]);
            // UTF-8-safe first-byte capitalization.
            if (f0 >= 'a' && f0 <= 'z') {
                monthName[0] = static_cast<char>(f0 - ('a' - 'A'));
            }
        }
        replaceAll(res, "_#C#_", monthName);
    }
    replaceAll(res, "_#X#_", day_names[wday]);
    replaceAll(res, "_#E#_", month_names_short[mon - 1]);
    replaceAll(res, "_#F#_", mon2Buf);
    replaceAll(res, "_#G#_", mon1Buf);
    replaceAll(res, "_#I#_", day2Buf);
    replaceAll(res, "_#J#_", day1Buf);
    replaceAll(res, "_#K#_", hr12_2);
    replaceAll(res, "_#L#_", hr12_1);
    replaceAll(res, "_#O#_", hr24_2);
    replaceAll(res, "_#P#_", hr24_1);
    replaceAll(res, "_#Q#_", min2Buf);
    replaceAll(res, "_#R#_", min1Buf);
    replaceAll(res, "_#S#_", day_names_short[wday]);
    replaceAll(res, "_#U#_", sec2Buf);
    replaceAll(res, "_#V#_", sec1Buf);
    replaceAll(res, "_#W#_", ap2Str);
    replaceAll(res, "_#Z#_", ap1Str);

    // Restore single-quoted literal segments.
    for (const auto &kv : quotedLiterals) {
        replaceAll(res, kv.first, kv.second);
    }

    return webstrada::string(res.c_str());
}

// True when `s` matches CF's date-string shape (dash/slash/dot separators with
// the right number of numeric fields), regardless of whether the components are
// in range. Used to distinguish CF's two invalid-date messages: a date-shaped
// string with an out-of-range component -> "X is an invalid date or time
// string."; a non-date string -> "The value X cannot be converted to a date."
// (was BUGS.md "CreateODBCDateTime invalid-date message").
bool looksLikeDateString(const webstrada::string &s) {
    webstrada::string str = s.trimmed();
    if (str.isEmpty()) return false;
    if (str.startWith("{ts '") && str.endsWith("'}")) return true;
    int year = 0, mon = 0, mday = 0, hour = 0, min = 0, sec = 0;
    int a = 0, b = 0, c = 0;
    if (std::sscanf(str.constData(), "%d-%d-%d %d:%d:%d", &year, &mon, &mday, &hour, &min, &sec) == 6) return true;
    if (std::sscanf(str.constData(), "%d-%d-%d", &year, &mon, &mday) == 3) return true;
    if (std::sscanf(str.constData(), "%d/%d/%d %d:%d:%d", &mon, &mday, &year, &hour, &min, &sec) == 6) return true;
    if (std::sscanf(str.constData(), "%d/%d/%d", &mon, &mday, &year) == 3) return true;
    if (std::sscanf(str.constData(), "%d.%d.%d %d:%d:%d", &a, &b, &c, &hour, &min, &sec) >= 3) return true;
    if (std::sscanf(str.constData(), "%d.%d.%d", &a, &b, &c) == 3) return true;
    return false;
}

bool parseDateTimeStr(const webstrada::string &s, double &days) {
    webstrada::string str = s.trimmed();
    if (str.isEmpty()) return false;

    int year = 0, mon = 0, mday = 0, hour = 0, min = 0, sec = 0;
    bool success = false;
    bool pm = false;
    bool isAm = false;

    // ISO-8601 timezone handling (CF's DateUtils.parseDateTime): a trailing
    // offset ("Z", "+HH", "+HHMM", "+HH:MM") turns the parsed wall clock into a
    // UTC instant, localized below to the server timezone. Only attempted when
    // the string carries a time marker (':') so a date-only string like
    // "2020-12-05" is not misread (its trailing "-05" is not an offset).
    int tzOffsetMin = 0;
    bool hasTz = false;
    if (str.contains(':')) {
        const char *p = str.constData() ? str.constData() : "";
        size_t len = str.length();
        size_t tzStart = len;
        if (len > 0 && (p[len - 1] == 'Z' || p[len - 1] == 'z')) {
            tzOffsetMin = 0;
            hasTz = true;
            tzStart = len - 1;
        } else {
            for (size_t i = len; i-- > 0; ) {
                if (p[i] == '+' || p[i] == '-') {
                    size_t k = i + 1, d = 0;
                    int hh = 0;
                    while (k < len && std::isdigit((unsigned char)p[k]) && d < 2) { hh = hh * 10 + (p[k] - '0'); k++; d++; }
                    if (d >= 1) {
                        int mm = 0;
                        if (k < len && p[k] == ':') {
                            k++;
                            size_t md = 0;
                            while (k < len && std::isdigit((unsigned char)p[k]) && md < 2) { mm = mm * 10 + (p[k] - '0'); k++; md++; }
                            if (md != 2) break;
                        } else if (k + 2 <= len && std::isdigit((unsigned char)p[k]) && std::isdigit((unsigned char)p[k + 1])) {
                            mm = (p[k] - '0') * 10 + (p[k + 1] - '0');
                            k += 2;
                        }
                        if (k == len && hh <= 14 && mm <= 59) {
                            tzOffsetMin = (p[i] == '+') ? (hh * 60 + mm) : -(hh * 60 + mm);
                            hasTz = true;
                            tzStart = i;
                        }
                    }
                    break;
                }
            }
        }
        if (hasTz) {
            webstrada::string base = str.left(tzStart).trimmed();
            if (base.isEmpty()) return false;
            // CF only accepts an offset after a time part; a date-only string
            // followed by "+05:00" is invalid (verified: IsDate("2020-05-15+05:00")
            // -> NO), even though the offset's own ':' triggered this block.
            if (!base.contains(':')) hasTz = false;
            else str = base;
        }
    }

    // Normalize the ISO-8601 'T' date/time separator ("2020-05-15T10:30:00") to
    // a space so the numeric scans below accept it. Only a 'T' flanked by
    // digits is touched (the ISO literal), never zone names or other text.
    {
        const char *cs = str.constData() ? str.constData() : "";
        size_t len = str.length();
        for (size_t i = 1; i + 1 < len; i++) {
            if (cs[i] == 'T' && cs[i - 1] >= '0' && cs[i - 1] <= '9' && cs[i + 1] >= '0' && cs[i + 1] <= '9') {
                str = str.left(i) + webstrada::string(" ") + str.mid(i + 1, len - i - 1);
                break;
            }
        }
    }

    // AM/PM suffix handling: DateUtils.parseDateTime accepts "10:30 AM" /
    // "2:30:45 PM" (12-hour clock); strip the marker and convert the hour.
    webstrada::string work = str;
    {
        int amLen = 0, pmLen = 0;
        webstrada::string upper = str;
        upper.toUpper();
        if (upper.endsWith("AM")) { amLen = 2; isAm = true; }
        if (upper.endsWith("PM")) { pmLen = 2; pm = true; }
        int cut = 0;
        if (amLen) cut = amLen;
        if (pmLen) cut = pmLen;
        if (cut) {
            work = str.left(str.length() - cut).trimmed();
        }
    }

    if (str.startWith("{ts '")) {
        // CF's parseTimeStamps parses up to the closing "}'" and ignores any
        // trailing text ({ts '...'}bogus -> valid, verified on the RDS host).
        int closeQuote = str.indexOf("'}", 5);
        if (closeQuote > 5) {
            webstrada::string sub = str.mid(5, closeQuote - 5);
            if (!sub.isEmpty()) {
                const char* cstr = sub.constData();
                if (cstr && std::sscanf(cstr, "%d-%d-%d %d:%d:%d", &year, &mon, &mday, &hour, &min, &sec) >= 3) {
                    success = true;
                }
            }
        }
    }
    else {
        // Separator-agnostic date parsing: dash/slash/dot all accept
        // YYYY-MM-DD, MM-DD-YYYY and DD-MM-YYYY with the same year-position
        // disambiguation (CF's DateUtils.parseDateTime, US locale; was a
        // divergence: dash dates only recognized year-first, so "12-31-2026"
        // failed while CF returns YES). A bare time (HH:MM[:SS], optionally
        // AM/PM) is also a valid date.
        int a = 0, b = 0, c = 0;
        int th = 0, tm = 0, ts = 0;
        bool timeMatched = false;
        bool hadSeconds = false;   // the time part carried :SS (fractional seconds allowed)
        bool hadDate = false;      // a date field was parsed (not a bare time)
        int consumed = 0;          // characters consumed by the successful scan
        const char *wp = work.constData() ? work.constData() : "";

        // Try the full 6-field (YYYY-MM-DD HH:MM:SS) dash form first. %n is
        // only written when the format is fully matched, so a partial (5-field)
        // match falls through to the dedicated 5-field scan below. The date and
        // time must be separated by whitespace (CF requires a delimiter; a
        // greedy %d would otherwise read "+05:00" / "-0530" attached to the
        // date as a sign-led time).
        int n1 = std::sscanf(wp, "%d-%d-%d%*[ \t\r\n]%d:%d:%d%n", &a, &b, &c, &th, &tm, &ts, &consumed);
        if (n1 == 6) {
            timeMatched = true;
            hadDate = true;
            hadSeconds = true;
        } else {
            // 5-field dash (date + HH:MM) and 3-field dash (date only).
            int pos5 = 0;
            if (std::sscanf(wp, "%d-%d-%d%*[ \t\r\n]%d:%d%n", &a, &b, &c, &th, &tm, &pos5) == 5) {
                timeMatched = true;
                hadDate = true;
                consumed = pos5;
            } else if (std::sscanf(wp, "%d-%d-%d%n", &a, &b, &c, &consumed) == 3) {
                hadDate = true;
            }
        }

        if (!timeMatched && !hadDate) {
            int n2 = std::sscanf(wp, "%d/%d/%d%*[ \t\r\n]%d:%d:%d%n", &a, &b, &c, &th, &tm, &ts, &consumed);
            if (n2 == 6) {
                timeMatched = true;
                hadDate = true;
                hadSeconds = true;
            } else if (n2 == 5) {
                timeMatched = true;
                hadDate = true;
                consumed = 0;
                int pos5b = 0;
                if (std::sscanf(wp, "%d/%d/%d%*[ \t\r\n]%d:%d%n", &a, &b, &c, &th, &tm, &pos5b) == 5) consumed = pos5b;
            }
            else if (std::sscanf(wp, "%d/%d/%d%n", &a, &b, &c, &consumed) == 3) {
                hadDate = true;
            }
            else if (std::sscanf(wp, "%d:%d:%d%n", &th, &tm, &ts, &consumed) == 3) {
                timeMatched = true;
                hadSeconds = true;
                a = 0; b = 0; c = 0;
            }
            else if (std::sscanf(wp, "%d:%d%n", &th, &tm, &consumed) == 2) {
                timeMatched = true;
                a = 0; b = 0; c = 0;
            }
            else if (std::sscanf(wp, "%d.%d.%d%*[ \t\r\n]%d:%d:%d%n", &a, &b, &c, &th, &tm, &ts, &consumed) >= 3) {
                // HH:MM or HH:MM:SS accepted (>= 3 fields, but the time part can
                // be 2 or 3 fields); distinguish a trailing time from a bare date
                // by re-scanning for the full six-field form.
                int n = std::sscanf(wp, "%d.%d.%d%*[ \t\r\n]%d:%d:%d%n", &a, &b, &c, &th, &tm, &ts, &consumed);
                if (n == 6) {
                    timeMatched = true;
                    hadDate = true;
                    hadSeconds = true;
                } else if (n == 5) {
                    timeMatched = true;
                    hadDate = true;
                    consumed = 0;
                    int pos5c = 0;
                    if (std::sscanf(wp, "%d.%d.%d%*[ \t\r\n]%d:%d%n", &a, &b, &c, &th, &tm, &pos5c) == 5) consumed = pos5c;
                } else if (n == 3) {
                    hadDate = true;
                    consumed = 0;
                    std::sscanf(wp, "%d.%d.%d%n", &a, &b, &c, &consumed);
                }
            }
            else if (std::sscanf(wp, "%d.%d.%d%n", &a, &b, &c, &consumed) == 3) {
                hadDate = true;
            }
            else {
                a = 0; b = 0; c = 0;
                consumed = 0;
            }
        }

        // Interpret the three numeric fields by year position:
        // - a 4-digit leading field is the year (YYYY.MM.DD).
        // - a 4-digit trailing field is the year: MM.DD.YYYY preferred over
        //   DD.MM.YYYY when the leading field is a valid month.
        // - all fields zero with a time (bare time HH:MM[:SS]) -> 1899-12-30.
        if (a >= 1000) {
            year = a; mon = b; mday = c;
            success = true;
        }
        else if (c >= 1000) {
            if (a >= 1 && a <= 12) {
                mon = a; mday = b; year = c;
            }
            else {
                mon = b; mday = a; year = c;
            }
            success = true;
        }
        else if (timeMatched && a == 0 && b == 0 && c == 0) {
            // Bare time (HH:MM[:SS]) — a valid date in CF.
            year = 1899; mon = 12; mday = 30;
            success = true;
        }
        // Time fields: a failed time-pattern scan can have written a partial
        // match into th/tm/ts; only trust them when timeMatched is set.
        if (success) {
            if (timeMatched) { hour = th; min = tm; sec = ts; }
            else { hour = 0; min = 0; sec = 0; }
        }

        // ---- Full-consumption check ----
        // CF's CFDateTimeParser requires the whole string to be consumed. The
        // numeric scans above are prefix-based, so without this a trailing
        // non-whitespace tail would be silently ignored (was BUGS.md
        // "IsDate()/ParseDateTime() ignore trailing garbage"). Verified against
        // CF 2025: "2020-05-15bogus" / "2020-05-15 10:30:00bogus" /
        // "2020-05-15Tbogus" -> NO, "2020-05-15 10:30:00 " -> YES, and a
        // trailing timezone offset is allowed. A "+HH:MM" / "-HH:MM" offset
        // that parses is followed by arbitrary garbage only in a full
        // date+time string (parseTime returns on !hoursGiven); a bare time
        // rejects the garbage. "Z" must be the final character.
        if (success) {
            size_t wlen = work.length();
            size_t pos = (size_t)consumed;
            bool tailOK = false;

            // Optional fractional seconds ".ddd" right after a seconds field.
            if (hadSeconds && pos < wlen && work.at(pos) == '.') {
                size_t q = pos + 1;
                size_t nd = 0;
                while (q < wlen && std::isdigit((unsigned char)work.at(q))) { q++; nd++; }
                if (nd >= 1) pos = q;
                else pos = wlen + 1; // "." with no digits -> invalid
            }

            if (pos <= wlen) {
                // Trailing whitespace is fine.
                size_t ws = pos;
                while (ws < wlen && std::isspace((unsigned char)work.at(ws))) ws++;
                if (ws == wlen) {
                    tailOK = true;
                } else if (timeMatched) {
                    // A trailing timezone offset: "Z" (must end the string) or
                    // "+HH[:MM]" / "+HHMM" / "-HH[:MM]" / "-HHMM".
                    char c0 = work.at(ws);
                    if (c0 == 'Z' || c0 == 'z') {
                        size_t q = ws + 1;
                        while (q < wlen && std::isspace((unsigned char)work.at(q))) q++;
                        tailOK = (q == wlen);
                    } else if (c0 == '+' || c0 == '-') {
                        size_t q = ws + 1;
                        size_t nd = 0;
                        int val = 0;
                        while (q < wlen && std::isdigit((unsigned char)work.at(q)) && nd < 4) {
                            val = val * 10 + (work.at(q) - '0'); q++; nd++;
                        }
                        bool offsetOK = false;
                        if (nd == 2 || nd == 4) {
                            // Bare 2-digit hour, or 4-digit HHMM (like +0530).
                            int offHours = (nd == 4) ? (val / 100) : val;
                            int offMin = (nd == 4) ? (val % 100) : 0;
                            if (offMin <= 59 && offHours <= (c0 == '+' ? 14 : 12)) {
                                if (q < wlen && work.at(q) == ':') {
                                    size_t r = q + 1;
                                    size_t md = 0;
                                    int mm = 0;
                                    while (r < wlen && std::isdigit((unsigned char)work.at(r)) && md < 2) {
                                        mm = mm * 10 + (work.at(r) - '0'); r++; md++;
                                    }
                                    if (md == 2 && mm <= 59) {
                                        offsetOK = true;
                                        q = r;
                                    }
                                } else {
                                    // No ":MM": the offset must end the string.
                                    size_t r = q;
                                    while (r < wlen && std::isspace((unsigned char)work.at(r))) r++;
                                    offsetOK = (r == wlen);
                                }
                            }
                        }
                        if (offsetOK) {
                            // A full date+time accepts any text after a parsed
                            // "+HH:MM" offset (CF's !hoursGiven short-circuit);
                            // a bare time requires the string to end.
                            if (hadDate) {
                                tailOK = true;
                            } else {
                                size_t r = q;
                                while (r < wlen && std::isspace((unsigned char)work.at(r))) r++;
                                tailOK = (r == wlen);
                            }
                        }
                    }
                }
            }
            if (!tailOK) success = false;
        }
    }

    // 12-hour AM/PM conversion (only when the input had an AM/PM marker):
    // PM: hours 1-11 -> +12 (12 PM stays 12; a 24h hour like 13 stays as-is);
    // AM: 12 -> 0 (midnight), others unchanged.
    if (success && pm) {
        if (hour >= 1 && hour <= 11) hour += 12;
    } else if (success && isAm) {
        if (hour == 12) hour = 0;
    }

    if (success) {
        if (mon < 1 || mon > 12 || mday < 1 || mday > 31 || hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59) {
            return false;
        }
        if (hasTz) {
            // The wall clock carried an explicit timezone: reduce it to a UTC
            // instant (a zone of +hh:mm is subtracted, e.g. 10:30+05:00 ->
            // 05:30 UTC), then localize it to the server timezone like CF
            // (2020-05-15T10:30:00Z on a UTC+2 host -> 12:30:00).
            long long totalMin = (long long)hour * 60 + min - tzOffsetMin;
            long long extraDays = (totalMin / 1440) - ((totalMin % 1440 < 0) ? 1 : 0);
            long long hh = ((totalMin % 1440) + 1440) % 1440;
            hour = (int)(hh / 60);
            min = (int)(hh % 60);

            struct tm tmUtc;
            memset(&tmUtc, 0, sizeof(tmUtc));
            tmUtc.tm_year = year - 1900;
            tmUtc.tm_mon = mon - 1;
            tmUtc.tm_mday = mday;
            tmUtc.tm_hour = hour;
            tmUtc.tm_min = min;
            tmUtc.tm_sec = sec;
            double utcDays = tmToDays(tmUtc) + extraDays;
            std::time_t epoch = static_cast<std::time_t>(std::llround((utcDays - 25569.0) * 86400.0));
            struct tm tmLocal;
            localtime_r(&epoch, &tmLocal);
            days = tmToDays(tmLocal);
        } else {
            struct tm tm;
            memset(&tm, 0, sizeof(tm));
            tm.tm_year = year - 1900;
            tm.tm_mon = mon - 1;
            tm.tm_mday = mday;
            tm.tm_hour = hour;
            tm.tm_min = min;
            tm.tm_sec = sec;

            days = tmToDays(tm);
        }
        return true;
    }
    return false;
}

namespace {

// Result of one Java SimpleDateFormat-style pattern attempt (mirrors
// SimpleDateFormat.parse(text, pos): a non-null Date + a ParsePosition, where
// errorIndex > 0 marks a partial match that failed mid-pattern).
struct DateParseResult {
    bool matched = false;   // parsed at least one field (Date != null)
    bool hadError = false;  // errorIndex > 0 (a later token failed to match)
    size_t consumed = 0;    // pos.index after the attempt
    int year = 0, month = 0; // month 0-based (Java Calendar convention)
    int day = 0, hour = 0, minute = 0, second = 0, millis = 0;
    int tzMinutes = 0;      // parsed timezone offset (0 when none)
    bool hasTz = false;     // an explicit timezone token (z/X) was parsed
    int wday = -1;          // parsed day-of-week name (0=Sunday), -1 when none
    bool hasYear = false, hasMonth = false, hasDay = false;
    bool hasHour = false, hasMinute = false, hasSecond = false;
    bool twelveHour = false; // hour token was 12-hour (h) rather than 24-hour (H)
    bool hasAmpm = false;
    int ampm = 0;           // 1=AM, 2=PM
};

bool utf8RegionEqualsIgnoreCase(const char *s, size_t len, size_t pos, const char *name, size_t nameLen) {
    if (pos + nameLen > len) return false;
    for (size_t i = 0; i < nameLen; i++) {
        unsigned char a = (unsigned char)s[pos + i];
        unsigned char b = (unsigned char)name[i];
        if (a != b) {
            if (a >= 'A' && a <= 'Z' && b >= 'a' && b <= 'z' && a == b - 32) continue;
            if (b >= 'A' && b <= 'Z' && a >= 'a' && a <= 'z' && b == a - 32) continue;
            return false;
        }
    }
    return true;
}

// Longest case-insensitive prefix match of any name in `names` at s[pos].
// Returns the index (0-based) or -1. (SimpleDateFormat keeps the longest match.)
int matchName(const char *s, size_t len, size_t pos, const char *const *names, int count) {
    int bestIdx = -1;
    size_t bestLen = 0;
    for (int i = 0; i < count; i++) {
        if (!names[i]) continue;
        size_t nLen = std::strlen(names[i]);
        if (nLen > bestLen && utf8RegionEqualsIgnoreCase(s, len, pos, names[i], nLen)) {
            bestIdx = i;
            bestLen = nLen;
        }
    }
    return bestIdx;
}

// AM/PM marker: the locale's markers plus the literal AM/PM (case-insensitive).
// Returns 1 (AM), 2 (PM) or -1, and sets *outLen to the matched length.
int matchAmpm(const char *s, size_t len, size_t pos, const cfml::LocaleInfo *loc, size_t *outLen) {
    const char *cands[4];
    cands[0] = loc ? loc->am : nullptr;
    cands[1] = loc ? loc->pm : nullptr;
    cands[2] = "AM";
    cands[3] = "PM";
    int best = -1;
    size_t bestLen = 0;
    for (int i = 0; i < 4; i++) {
        if (!cands[i]) continue;
        size_t nLen = std::strlen(cands[i]);
        if (nLen > bestLen && utf8RegionEqualsIgnoreCase(s, len, pos, cands[i], nLen)) {
            best = i < 2 ? i + 1 : (cands[i][0] == 'A' || cands[i][0] == 'a' ? 1 : 2);
            bestLen = nLen;
        }
    }
    if (best > 0 && outLen) *outLen = bestLen;
    return best;
}

// Consume a `z` timezone token: GMT/GMT+HH[:MM]/UTC/UT or a name token.
// Returns the offset in minutes (positive = east of UTC), INT_MIN when nothing
// matches (a negative return value is a valid west-of-UTC offset, not a failure).
int matchZoneOffset(const char *s, size_t len, size_t pos) {
    const char *known[] = {"GMT", "UTC", "UT", "CET", "CEST", "EET", "EEST", "WET", "WEST",
                           "PST", "PDT", "MST", "MDT", "EST", "EDT", "CST", "CDT", "HST", "AKST", "AKDT", "AST"};
    const int knownOff[] = {0, 0, 0, 60, 120, 120, 180, 0, 60, -480, -420, -420, -360, -300, -240, -360, -300, -600, -540, -480, -240};
    for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
        size_t nLen = std::strlen(known[i]);
        if (utf8RegionEqualsIgnoreCase(s, len, pos, known[i], nLen)) {
            if (i == 0 && pos + nLen < len && (s[pos + nLen] == '+' || s[pos + nLen] == '-')) {
                bool plus = s[pos + nLen] == '+';
                size_t k = pos + nLen + 1;
                size_t d = 0;
                int hh = 0;
                while (k < len && std::isdigit((unsigned char)s[k]) && d < 2) { hh = hh * 10 + (s[k] - '0'); k++; d++; }
                if (d == 0) return INT_MIN;
                int mm = 0;
                if (k < len && s[k] == ':') {
                    k++;
                    size_t md = 0;
                    while (k < len && std::isdigit((unsigned char)s[k]) && md < 2) { mm = mm * 10 + (s[k] - '0'); k++; md++; }
                    if (md == 0) return INT_MIN;
                }
                int off = hh * 60 + mm;
                return plus ? off : -off;
            }
            return knownOff[i];
        }
    }
    return INT_MIN;
}

// Consume an ISO-8601 offset (X/XXX): "Z", "+HH", "+HHMM", "+HH:MM" (or minus).
// Returns offset minutes (positive = east), INT_MIN when nothing matches.
int matchIsoOffset(const char *s, size_t len, size_t pos) {
    if (pos < len && (s[pos] == 'Z' || s[pos] == 'z')) return 0;
    if (pos >= len || (s[pos] != '+' && s[pos] != '-')) return INT_MIN;
    bool plus = s[pos] == '+';
    size_t k = pos + 1;
    size_t d = 0;
    int hh = 0;
    while (k < len && std::isdigit((unsigned char)s[k]) && d < 2) { hh = hh * 10 + (s[k] - '0'); k++; d++; }
    if (d == 0) return INT_MIN;
    int mm = 0;
    if (k < len && s[k] == ':') {
        k++;
        size_t md = 0;
        while (k < len && std::isdigit((unsigned char)s[k]) && md < 2) { mm = mm * 10 + (s[k] - '0'); k++; md++; }
        if (md != 2) return INT_MIN;
    } else if (d == 2 && k + 2 <= len && std::isdigit((unsigned char)s[k]) && std::isdigit((unsigned char)s[k + 1])) {
        mm = (s[k] - '0') * 10 + (s[k + 1] - '0');
    }
    if (hh > 14) return INT_MIN;
    int off = hh * 60 + mm;
    return plus ? off : -off;
}

// Match a Java SimpleDateFormat-style pattern starting at `start`. On return,
// out.consumed is the number of input bytes consumed. A pattern whose first
// token fails leaves matched=false; a pattern that parsed at least one token
// returns matched=true even if it stopped early (SimpleDateFormat returns a
// Date for partial matches). out.hadError marks a partial match that failed on
// a later token (errorIndex > 0).
void matchDatePattern(const char *s, size_t len, size_t start, const std::string &pattern,
                      const cfml::LocaleInfo *loc, DateParseResult &out) {
    DateParseResult r;
    size_t pos = start;
    size_t i = 0;
    size_t n = pattern.size();
    bool valid = true;
    bool parsedAny = false;
    bool hadError = false;

    while (i < n && valid) {
        char c = pattern[i];
        if (c == '\'') {
            i++;
            bool closed = false;
            while (i < n && valid) {
                if (pattern[i] == '\'') {
                    if (i + 1 < n && pattern[i + 1] == '\'') {
                        if (pos >= len || s[pos] != '\'') { valid = false; hadError = true; break; }
                        pos++;
                        i += 2;
                    } else {
                        i++;
                        closed = true;
                        break;
                    }
                } else {
                    if (pos >= len || s[pos] != pattern[i]) { valid = false; hadError = true; break; }
                    pos++;
                    i++;
                }
            }
            if (!closed) valid = false;
            if (valid) parsedAny = true;
            continue;
        }
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            size_t j = i;
            while (j < n && pattern[j] == c) j++;
            int count = (int)(j - i);
            bool isNum = false;
            if (c == 'M' && count >= 3) {
                // Month name (MMMM / MMM): longest of full/short names.
                int mo1 = matchName(s, len, pos, loc->months, 12);
                int mo2 = matchName(s, len, pos, loc->monthsShort, 12);
                int mo = -1;
                size_t mlen = 0;
                if (mo1 >= 0) { mo = mo1; mlen = std::strlen(loc->months[mo1]); }
                if (mo2 >= 0 && std::strlen(loc->monthsShort[mo2]) > mlen) { mo = mo2; mlen = std::strlen(loc->monthsShort[mo2]); }
                if (mo < 0) { valid = false; hadError = true; }
                else { r.month = mo; r.hasMonth = true; pos += mlen; parsedAny = true; }
            } else if (c == 'y' || c == 'Y' || c == 'M' || c == 'm' || c == 'd' || c == 'H' || c == 'k' ||
                c == 'h' || c == 'K' || c == 's' || c == 'S') {
                size_t d = 0;
                long long val = 0;
                while (pos < len && std::isdigit((unsigned char)s[pos])) { val = val * 10 + (s[pos] - '0'); pos++; d++; }
                if (d == 0) { valid = false; hadError = true; }
                else {
                    isNum = true;
                    if (c == 'y' || c == 'Y') {
                        if (c == 'y' && count == 2 && val < 100) {
                            time_t now = std::time(nullptr);
                            struct tm tmv;
                            memset(&tmv, 0, sizeof(tmv));
                            localtime_r(&now, &tmv);
                            int base = tmv.tm_year + 1900 - 80; // JDK defaultCenturyStart = now - 80y
                            int century = base - (base % 100);
                            int y2 = century + (int)val;
                            if (y2 < base) y2 += 100;
                            val = y2;
                        }
                        r.year = (int)val;
                        r.hasYear = true;
                    } else if (c == 'M') {
                        r.month = (int)val - 1;
                        r.hasMonth = true;
                    } else if (c == 'm') {
                        r.minute = (int)val;
                        r.hasMinute = true;
                    } else if (c == 'd') {
                        r.day = (int)val;
                        r.hasDay = true;
                    } else if (c == 'H' || c == 'k') {
                        r.hour = (int)val;
                        r.hasHour = true;
                    } else if (c == 'h' || c == 'K') {
                        r.hour = (int)val;
                        r.hasHour = true;
                        r.twelveHour = true;
                    } else if (c == 's') {
                        r.second = (int)val;
                        r.hasSecond = true;
                    } else { // 'S': milliseconds = raw digits; >999 out of range
                        if (val > 999) { valid = false; hadError = true; }
                        else { r.millis = (int)val; }
                    }
                    parsedAny = true;
                }
            } else if (c == 'E' || c == 'e' || c == 'c') {
                int w1 = matchName(s, len, pos, loc->days, 7);
                int w2 = matchName(s, len, pos, loc->daysShort, 7);
                int w = -1;
                size_t wlen = 0;
                if (w1 >= 0) { w = w1; wlen = std::strlen(loc->days[w1]); }
                if (w2 >= 0 && std::strlen(loc->daysShort[w2]) > wlen) { w = w2; wlen = std::strlen(loc->daysShort[w2]); }
                if (w < 0) { valid = false; hadError = true; }
                else { r.wday = w; pos += wlen; parsedAny = true; }
            } else if (c == 'a') {
                size_t aLen = 0;
                int ap = matchAmpm(s, len, pos, loc, &aLen);
                if (ap < 0) { valid = false; hadError = true; }
                else { r.ampm = ap; r.hasAmpm = true; pos += aLen; parsedAny = true; }
            } else if (c == 'z') {
                int tz = matchZoneOffset(s, len, pos);
                if (tz == INT_MIN) { valid = false; hadError = true; }
                else {
                    size_t k = pos;
                    while (k < len && (std::isalnum((unsigned char)s[k]) || s[k] == ':' || s[k] == '+' || s[k] == '-' || s[k] == '/')) k++;
                    if (k == pos) { valid = false; hadError = true; }
                    else { pos = k; r.tzMinutes = tz; r.hasTz = true; parsedAny = true; }
                }
            } else if (c == 'X') {
                int tz = matchIsoOffset(s, len, pos);
                if (tz == INT_MIN) { valid = false; hadError = true; }
                else {
                    size_t k = pos;
                    if (k < len && (s[k] == 'Z' || s[k] == 'z')) k++;
                    else { while (k < len && (std::isalnum((unsigned char)s[k]) || s[k] == ':' || s[k] == '+' || s[k] == '-')) k++; }
                    if (k == pos) { valid = false; hadError = true; }
                    else { pos = k; r.tzMinutes = tz; r.hasTz = true; parsedAny = true; }
                }
            } else {
                // unrecognized pattern letter: literal match
                if (pos >= len || s[pos] != c) { valid = false; hadError = true; }
                else { pos++; parsedAny = true; }
            }
            i = j;
        } else {
            // literal (punctuation / whitespace)
            if (pos >= len || s[pos] != c) { valid = false; hadError = true; }
            else { pos++; parsedAny = true; }
            i++;
        }
    }

    if (!valid) {
        // A failed token makes SimpleDateFormat.parse return null and reset the
        // ParsePosition to its start position (even if earlier tokens matched).
        out = r;
        out.matched = false;
        out.hadError = true;
        out.consumed = start;
        return;
    }
    r.matched = parsedAny;
    r.hadError = hadError;
    r.consumed = pos;

    // Range validation (SimpleDateFormat with lenient=false rejects out-of-range
    // fields; Calendar.set throws and parse() returns null).
    if (r.hasMonth && (r.month < 0 || r.month > 11)) { out = r; out.matched = false; out.consumed = start; return; }
    if (r.hasDay && (r.day < 1 || r.day > 31)) { out = r; out.matched = false; out.consumed = start; return; }
    if (r.hasHour) {
        if (r.twelveHour && (r.hour < 1 || r.hour > 12)) { out = r; out.matched = false; out.consumed = start; return; }
        if (!r.twelveHour && (r.hour < 0 || r.hour > 23)) { out = r; out.matched = false; out.consumed = start; return; }
    }
    if (r.hasMinute && (r.minute < 0 || r.minute > 59)) { out = r; out.matched = false; out.consumed = start; return; }
    if (r.hasSecond && (r.second < 0 || r.second > 59)) { out = r; out.matched = false; out.consumed = start; return; }
    if (r.hasYear && r.hasMonth && r.hasDay) {
        static const int dim[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        int y = r.year;
        int m = r.month;
        int maxd = dim[m];
        if (m == 1 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) maxd = 29;
        if (r.day > maxd) { out = r; out.matched = false; out.consumed = start; return; }
        // Day-of-week consistency (lenient=false rejects a conflicting name).
        if (r.wday >= 0) {
            int a = (14 - (m + 1)) / 12;
            int yy = y + 4800 - a;
            int mm = (m + 1) + 12 * a - 3;
            int jdn = r.day + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
            int w = (jdn + 1) % 7; // 0=Sunday
            if (w != r.wday) { out = r; out.matched = false; out.consumed = start; return; }
        }
    }
    // Convert 12-hour + AM/PM into 24-hour hour.
    if (r.hasAmpm && r.hasHour) {
        if (r.ampm == 2 && r.hour != 12) r.hour += 12;
        else if (r.ampm == 1 && r.hour == 12) r.hour = 0;
    }
    out = r;
}

} // namespace

bool parseDateTimeLocale(const webstrada::string &s, const cfml::LocaleInfo *locale, double &days) {
    if (!locale) locale = cfml::locale_default();
    std::string st = s.constData() ? s.constData() : "";
    {
        size_t b = st.find_first_not_of(" \t\r\n");
        size_t e = st.find_last_not_of(" \t\r\n");
        if (b == std::string::npos) return false;
        st = st.substr(b, e - b + 1);
    }
    size_t len = st.length();

    std::string dS = locale->pDateShort ? locale->pDateShort : "";
    std::string dMe = locale->pDateMedium ? locale->pDateMedium : "";
    std::string dLo = locale->pDateLong ? locale->pDateLong : "";
    std::string dFu = locale->pDateFull ? locale->pDateFull : "";
    std::string tS = locale->pTimeShort ? locale->pTimeShort : "";
    std::string tMe = locale->pTimeMedium ? locale->pTimeMedium : "";
    std::string tLo = locale->pTimeLong ? locale->pTimeLong : "";
    std::string tFu = locale->pTimeFull ? locale->pTimeFull : "";

    // getDateTimeFormatters()
    std::vector<std::string> dtPats;
    dtPats.push_back(dS + " " + tS);
    dtPats.push_back(dMe + " " + tMe);
    dtPats.push_back(dLo + " " + tLo);
    dtPats.push_back(dFu + " " + tFu);
    dtPats.push_back("EEE, dd MMM yyyy HH:mm:ss zzz");
    dtPats.push_back("yyyy-MM-dd'T'HH:mm:ssXXX");
    dtPats.push_back("yyyy-MM-dd'T'HH:mm:ss");

    // getDateAndTimeFormatters(): {dateShort, dateMedium, dateFull, dateLong,
    //   yyyy-MM-dd, dd.MM.yyyy, dd/MM/yyyy, timeFull, timeLong, H:mm:ss.S,
    //   h:mm:ss a, H:mm:ss, h:mm a, H:mm, timeMedium, timeShort}
    std::vector<std::string> dsPats;
    dsPats.push_back(dS);
    dsPats.push_back(dMe);
    dsPats.push_back(dFu);
    dsPats.push_back(dLo);
    dsPats.push_back("yyyy-MM-dd");
    dsPats.push_back("dd.MM.yyyy");
    dsPats.push_back("dd/MM/yyyy");
    dsPats.push_back(tFu);
    dsPats.push_back(tLo);
    dsPats.push_back("H:mm:ss.S");
    dsPats.push_back("h:mm:ss a");
    dsPats.push_back("H:mm:ss");
    dsPats.push_back("h:mm a");
    dsPats.push_back("H:mm");
    dsPats.push_back(tMe);
    dsPats.push_back(tS);

    DateParseResult dt, d, t;
    bool dtSet = false, dSet = false, tSet = false;
    size_t pos = 0;

    // Phase 1: datetime formatters; stop at the first that parses anything.
    for (size_t p = 0; p < dtPats.size(); p++) {
        DateParseResult m;
        matchDatePattern(st.c_str(), len, pos, dtPats[p], locale, m);
        if (m.matched) {
            dt = m;
            dtSet = true;
            pos = m.consumed;
            break;
        }
    }

    if (!dtSet || dt.consumed != len) {
        // Phase 2: date + time formatters with the shared ParsePosition.
        pos = 0;
        for (int i = 0; i < 16; i++) {
            DateParseResult m;
            if (i < 7) {
                if (!dSet) {
                    matchDatePattern(st.c_str(), len, pos, dsPats[i], locale, m);
                    if (m.matched) {
                        d = m;
                        dSet = true;
                        pos = m.consumed;
                        if (d.consumed == len && d.hadError) { pos = 0; dSet = false; }
                        if (pos >= len) break;
                    }
                }
            } else {
                if (!tSet) {
                    matchDatePattern(st.c_str(), len, pos, dsPats[i], locale, m);
                    if (m.matched) {
                        t = m;
                        tSet = true;
                        pos = m.consumed;
                        if (t.consumed == len && t.hadError) { pos = 0; tSet = false; }
                        if (pos >= len) break;
                    }
                }
            }
        }

        size_t posIndex = pos;
        size_t strlen = len;
        if (posIndex < strlen && posIndex > 0) {
            std::string mDate = st.substr(posIndex);
            {
                size_t b = mDate.find_first_not_of(" \t\r\n");
                size_t e = mDate.find_last_not_of(" \t\r\n");
                if (b == std::string::npos) mDate.clear();
                else mDate = mDate.substr(b, e - b + 1);
            }
            size_t mlen = mDate.length();
            pos = 0;
            for (int i2 = 0; i2 < 16; i2++) {
                DateParseResult m;
                if (i2 < 7) {
                    if (!dSet) {
                        matchDatePattern(mDate.c_str(), mlen, pos, dsPats[i2], locale, m);
                        if (m.matched) {
                            d = m;
                            dSet = true;
                            pos = m.consumed;
                            if (d.consumed == mlen && d.hadError) { pos = 0; dSet = false; }
                            if (dSet && pos >= mlen) { posIndex = posIndex + pos + 1; break; }
                        }
                    }
                } else {
                    if (!tSet) {
                        matchDatePattern(mDate.c_str(), mlen, pos, dsPats[i2], locale, m);
                        if (m.matched) {
                            t = m;
                            tSet = true;
                            pos = m.consumed;
                            if (t.consumed == mlen && t.hadError) { pos = 0; tSet = false; }
                            if (tSet && pos >= mlen) { posIndex = posIndex + pos + 1; break; }
                        }
                    }
                }
            }
        }
        if (posIndex != len) return false;
    }

    // Combine the parsed pieces the way CFLocaleBase does.
    bool bDateSet = dSet;
    bool bTimeSet = tSet;
    int month = 0, date = 0, year = 0, hour = 0, min = 0, sec = 0, ms = 0, tzMin = 0;
    if (tSet) { hour = t.hour; min = t.minute; sec = t.second; ms = t.millis; tzMin += t.tzMinutes; }
    if (dSet) { month = d.month; date = d.day; year = d.year; tzMin += d.tzMinutes; }
    if (dtSet && !(bDateSet && bTimeSet)) {
        if (!bDateSet) { month = dt.month; date = dt.day; year = dt.year; bDateSet = true; }
        if (!bTimeSet) { hour = dt.hour; min = dt.minute; sec = dt.second; ms = dt.millis; bTimeSet = true; }
        tzMin += dt.tzMinutes;
    }
    // CF's time-only default date is year=-1 in OleDateTime units, which is the
    // calendar year 1899 (the OLE epoch 1899-12-30, day 0).
    if (!bDateSet) { month = 11; date = 30; year = 1899; }
    if (!bTimeSet) { hour = 0; min = 0; sec = 0; }
    if (!bTimeSet && !bDateSet) return false;

    // Apply the parsed timezone offset: reduce the wall clock to UTC (a zone of
    // +hh:mm is subtracted, e.g. 10:30+05:00 -> 05:30).
    bool hadTz = (dtSet && dt.hasTz) || (dSet && d.hasTz) || (tSet && t.hasTz);
    long long totalMin = (long long)hour * 60 + min - tzMin;
    long long extraDays = (totalMin / 1440) - ((totalMin % 1440 < 0) ? 1 : 0);
    int hh = (int)(((totalMin % 1440) + 1440) % 1440);
    hour = hh / 60;
    min = hh % 60;

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = year - 1900;
    tm.tm_mon = month;
    tm.tm_mday = date;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;

    if (hadTz) {
        // The string carried an explicit timezone, so the reduced components are
        // a UTC instant. CF converts the parsed instant to the server's local
        // timezone (2020-05-15T10:30:00Z on a UTC+2 server -> 12:30:00).
        double utcDays = tmToDays(tm) + extraDays;
        std::time_t epoch = static_cast<std::time_t>(std::llround((utcDays - 25569.0) * 86400.0));
        struct tm tmLocal;
        localtime_r(&epoch, &tmLocal);
        days = tmToDays(tmLocal) + ms / 86400000.0;
    } else {
        // No timezone in the string: the wall clock is already server-local.
        days = tmToDays(tm) + extraDays + ms / 86400000.0;
    }
    return true;
}

bool getDaysFromVariant(const cfvariant *arg, double &days) {
    if (!arg) return false;
    if (arg->m_type == cfvariant::DateTime) {
        days = arg->m_double;
        return true;
    } else if (arg->m_type == cfvariant::Number) {
        days = arg->m_int;
        return true;
    } else if (arg->m_type == cfvariant::Long) {
        days = static_cast<double>(arg->m_long);
        return true;
    } else if (arg->m_type == cfvariant::Float) {
        days = arg->m_double;
        return true;
    } else {
        return parseDateTimeStr(const_cast<cfvariant*>(arg)->toString(), days);
    }
}

double getDaysOrThrow(const cfvariant *arg, const char *funcName) {
    double days = 0.0;
    if (!getDaysFromVariant(arg, days)) {
        throw webstrada::exception(webstrada::string(funcName) + ": Invalid date/time value");
    }
    return days;
}

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int getDaysInMonthVal(int year, int mon) {
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (mon == 1) { // February
        return isLeapYear(year) ? 29 : 28;
    }
    if (mon >= 0 && mon < 12) {
        return days[mon];
    }
    return 30;
}

void normalizeTm(struct tm &tm) {
    while (tm.tm_mon < 0) {
        tm.tm_mon += 12;
        tm.tm_year--;
    }
    while (tm.tm_mon >= 12) {
        tm.tm_mon -= 12;
        tm.tm_year++;
    }
    int maxDays = getDaysInMonthVal(tm.tm_year + 1900, tm.tm_mon);
    if (tm.tm_mday > maxDays) {
        tm.tm_mday = maxDays;
    }
    double days = tmToDays(tm);
    tm = daysToTm(days);
}

// ---- from fn_query ----

cfvariant queryBuildRowStruct(const QueryData *qd, int rowIndex)
{
    cfvariant row(cfvariant::Struct);
    if (qd) {
        for (const auto &col : qd->columns) {
            row.structSet(col.name, col.values[rowIndex]);
        }
    }
    return row;
}

string queryColJavaTypeName(QueryData *qd, int colIdx, int rowIdx)
{
    if (qd && colIdx >= 0 && colIdx < (int)qd->columns.size()) {
        const QueryColumn &col = qd->columns[colIdx];
        if (rowIdx >= 0 && rowIdx < (int)col.values.size()) {
            switch (col.values[rowIdx].m_type) {
            case cfvariant::Number: return "java.lang.Integer";
            case cfvariant::Long:   return "java.lang.Long";
            case cfvariant::Float:  return "java.lang.Double";
            case cfvariant::Boolean:return "java.lang.Boolean";
            case cfvariant::DateTime:return "java.sql.Date";
            case cfvariant::String: return "java.lang.String";
            default: break;
            }
        }
        string t = col.type;
        t.toUpper();
        t = t.trimmed();
        if (t.equals("INTEGER") || t.equals("INT") || t.equals("TINYINT") || t.equals("SMALLINT"))
            return "java.lang.Integer";
        if (t.equals("BIGINT")) return "java.lang.Long";
        if (t.equals("DECIMAL") || t.equals("NUMERIC") || t.equals("DOUBLE") || t.equals("FLOAT") || t.equals("REAL"))
            return "java.lang.Double";
        if (t.equals("BOOLEAN") || t.equals("BIT")) return "java.lang.Boolean";
        if (t.equals("DATE") || t.equals("TIME") || t.equals("TIMESTAMP") || t.equals("DATETIME"))
            return "java.sql.Date";
    }
    return "java.lang.String";
}

cfvariant coerceQueryCell(const string &type, const cfvariant &raw)
{
    string t = type;
    t.toUpper();
    t = t.trimmed();
    if (raw.m_type == cfvariant::Null) return raw;
    if (t.isEmpty() || t.equals("VARCHAR") || t.equals("CHAR") || t.equals("STRING") ||
        t.equals("BLOB") || t.equals("CLOB") || t.equals("OBJECT") || t.equals("UUID")) {
        return raw;
    }
    if (t.equals("INTEGER") || t.equals("INT") || t.equals("TINYINT") || t.equals("SMALLINT")) {
        // CF truncates decimal strings to int ("30.9" -> 30) but rejects
        // non-numeric input ("abc" -> "Invalid data abc for CFSQLTYPE
        // CF_SQL_INTEGER."), verified against CF 2021.
        if (raw.m_type == cfvariant::Number || raw.m_type == cfvariant::Long || raw.m_type == cfvariant::Float) {
            return cfvariant(getIntValue(raw));
        }
        const char *str = variantToString(raw).constData();
        while (str && *str && isspace(*str)) str++;
        char *end = nullptr;
        long v = strtol(str ? str : "", &end, 10);
        while (end && *end && isspace(*end)) end++;
        if (!str || *str == '\0' || end == str) {
            throw webstrada::exception("Invalid data " + variantToString(raw) + " for CFSQLTYPE CF_SQL_INTEGER.");
        }
        // Reject a non-numeric remainder only when it isn't a decimal/fraction
        // (CF accepts "30.9" -> 30 but rejects "30abc").
        const char *r = end;
        if (*r == '.' || *r == ',') {
            r++;
            while (*r && isdigit((unsigned char)*r)) r++;
            while (*r && isspace(*r)) r++;
            if (*r != '\0') {
                throw webstrada::exception("Invalid data " + variantToString(raw) + " for CFSQLTYPE CF_SQL_INTEGER.");
            }
        } else if (*r != '\0') {
            throw webstrada::exception("Invalid data " + variantToString(raw) + " for CFSQLTYPE CF_SQL_INTEGER.");
        }
        return cfvariant(static_cast<int>(v));
    }
    if (t.equals("BIGINT")) {
        long long v = getLongIntValue(raw);
        cfvariant ret(cfvariant::Long);
        ret.m_long = v;
        return ret;
    }
    if (t.equals("DECIMAL") || t.equals("NUMERIC") || t.equals("DOUBLE") || t.equals("FLOAT") ||
        t.equals("REAL")) {
        double d = 0.0;
        try {
            d = getDoubleValue(raw);
        } catch (webstrada::exception &) {
            throw webstrada::exception("QueryNew: Invalid " + t + " value");
        }
        cfvariant ret(cfvariant::Float);
        ret.m_double = d;
        // A DECIMAL/NUMERIC cell keeps its original scale like CF's
        // BigDecimal (12.50, not 12.5); computed doubles (DOUBLE/FLOAT) do
        // not preserve source text. A string value's text is the scale source
        // too ("12.50" -> "12.50").
        if (t.equals("DECIMAL") || t.equals("NUMERIC")) {
            if (raw.m_literalText) {
                ret.m_literalText = new string(*raw.m_literalText);
            } else if (raw.m_type == cfvariant::String) {
                string text = variantToString(raw);
                if (!text.isEmpty()) ret.m_literalText = new string(text);
            }
        }
        return ret;
    }
    if (t.equals("DATE") || t.equals("TIME") || t.equals("TIMESTAMP") || t.equals("DATETIME")) {
        double days = 0.0;
        if (!getDaysFromVariant(&raw, days)) throw webstrada::exception("QueryNew: Invalid " + t + " value");
        cfvariant ret(cfvariant::DateTime);
        ret.m_double = days;
        return ret;
    }
    if (t.equals("BOOLEAN") || t.equals("BIT")) {
        cfvariant ret(cfvariant::Boolean);
        ret.m_bool = cfml::cfvariant_is_truthy(&raw) != 0;
        return ret;
    }
    return raw;
}

string queryColumnList(const cfvariant *query)
{
    if (!query || query->m_type != cfvariant::Query || !query->m_query) return string("");
    std::vector<string> names;
    for (auto &col : query->m_query->columns) {
        string n = col.name;
        n.toUpper();
        names.push_back(n);
    }
    std::sort(names.begin(), names.end(), [](const string &a, const string &b) {
        return a.compareCaseInsensitive(b) < 0;
    });
    string ret;
    for (size_t i = 0; i < names.size(); i++) {
        if (i > 0) ret += ",";
        ret += names[i];
    }
    return ret;
}

int queryRecordCount(const cfvariant *query)
{
    if (!query || query->m_type != cfvariant::Query || !query->m_query) return 0;
    return query->m_query->rowCount();
}

// ---- from fn_crypto ----

int binaryDecodeHexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void binaryDecodeHex(const webstrada::string &s, std::vector<std::byte> &out) {
    const char *data = s.constData();
    int len = s.length();
    int hi = -1;
    for (int i = 0; i < len; i++) {
        char c = data[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        int v = binaryDecodeHexVal(c);
        if (v < 0) {
            throw webstrada::exception("BinaryDecode: Invalid hex character '" + webstrada::string(1, c) + "'");
        }
        if (hi < 0) {
            hi = v;
        } else {
            out.push_back(std::byte((hi << 4) | v));
            hi = -1;
        }
    }
    if (hi >= 0) {
        throw webstrada::exception("BinaryDecode: Hex string must contain an even number of characters");
    }
}

int binaryDecodeBase64Val(char c, bool urlSafe) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (urlSafe) {
        if (c == '-') return 62;
        if (c == '_') return 63;
    }
    return -1;
}

void binaryDecodeBase64(const webstrada::string &s, std::vector<std::byte> &out, bool urlSafe) {
    const char *data = s.constData();
    int len = s.length();
    int i = 0;
    while (i < len) {
        if (data[i] == '=') {
            // Padding is only allowed at the very end (at most 2 characters)
            for (int j = i + 1; j < len; j++) {
                if (data[j] != '=') {
                    throw webstrada::exception("BinaryDecode: Invalid base64 padding");
                }
            }
            if (len - i > 2) {
                throw webstrada::exception("BinaryDecode: Invalid base64 padding");
            }
            return;
        }
        int c0 = binaryDecodeBase64Val(data[i], urlSafe);
        if (c0 < 0) {
            throw webstrada::exception("BinaryDecode: Invalid base64 character '" + webstrada::string(1, data[i]) + "'");
        }
        if (i + 1 >= len) {
            throw webstrada::exception("BinaryDecode: Invalid base64 length");
        }
        int c1 = binaryDecodeBase64Val(data[i + 1], urlSafe);
        if (c1 < 0) {
            throw webstrada::exception("BinaryDecode: Invalid base64 character '" + webstrada::string(1, data[i + 1]) + "'");
        }
        unsigned int acc = (c0 << 18) | (c1 << 12);
        out.push_back(std::byte((acc >> 16) & 0xFF));
        i += 2;
        if (i < len && data[i] != '=') {
            int c2 = binaryDecodeBase64Val(data[i], urlSafe);
            if (c2 < 0) {
                throw webstrada::exception("BinaryDecode: Invalid base64 character '" + webstrada::string(1, data[i]) + "'");
            }
            acc |= (c2 << 6);
            out.push_back(std::byte((acc >> 8) & 0xFF));
            i += 1;
            if (i < len && data[i] != '=') {
                int c3 = binaryDecodeBase64Val(data[i], urlSafe);
                if (c3 < 0) {
                    throw webstrada::exception("BinaryDecode: Invalid base64 character '" + webstrada::string(1, data[i]) + "'");
                }
                acc |= c3;
                out.push_back(std::byte(acc & 0xFF));
                i += 1;
            }
        }
    }
}

void binaryDecodeUU(const webstrada::string &s, std::vector<std::byte> &out) {
    const char *data = s.constData();
    int len = s.length();
    int i = 0;
    while (i < len) {
        // Skip line separators
        while (i < len && (data[i] == '\n' || data[i] == '\r')) i++;
        if (i >= len) break;
        int lineStart = i;
        while (i < len && data[i] != '\n' && data[i] != '\r') i++;
        int lineEnd = i;
        int lineLen = lineEnd - lineStart;

        // Skip "begin ..." and "end" marker lines
        if (lineLen >= 5 && strncasecmp(data + lineStart, "begin", 5) == 0) continue;
        if (lineLen >= 3 && strncasecmp(data + lineStart, "end", 3) == 0) continue;

        const char *line = data + lineStart;
        unsigned char lenCh = static_cast<unsigned char>(line[0]);
        if (lenCh < 32 || lenCh > 96) {
            throw webstrada::exception("BinaryDecode: Invalid UU line length character");
        }
        int n = (lenCh - 32) & 0x3F;
        if (n == 0) break; // end-of-data marker line

        int pos = 1;
        int produced = 0;
        while (pos < lineLen && produced < n) {
            int c[4];
            int got = 0;
            while (got < 4 && pos < lineLen) {
                unsigned char uc = static_cast<unsigned char>(line[pos++]);
                if (uc < 32 || uc > 96) continue; // skip control and out-of-range chars
                c[got++] = (uc - 32) & 0x3F;
            }
            if (got == 0) break;
            if (got >= 2) {
                out.push_back(std::byte(((c[0] << 2) | (c[1] >> 4)) & 0xFF));
                produced++;
            }
            if (got >= 3 && produced < n) {
                out.push_back(std::byte((((c[1] & 0xF) << 4) | (c[2] >> 2)) & 0xFF));
                produced++;
            }
            if (got >= 4 && produced < n) {
                out.push_back(std::byte((((c[2] & 0x3) << 6) | c[3]) & 0xFF));
                produced++;
            }
        }
    }
}

void binaryEncodeHex(const std::vector<std::byte> &in, webstrada::string &out) {
    for (auto b : in) {
        unsigned char c = static_cast<unsigned char>(b);
        out.append(cfml::cryptoHexDigits[c >> 4]);
        out.append(cfml::cryptoHexDigits[c & 0xF]);
    }
}

void binaryEncodeBase64(const std::vector<std::byte> &in, webstrada::string &out, bool urlSafe) {
    static const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static const char *b64url = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    const char *alphabet = urlSafe ? b64url : b64;
    size_t i = 0;
    size_t n = in.size();
    while (i + 2 < n) {
        unsigned int v = (static_cast<unsigned int>(static_cast<unsigned char>(in[i])) << 16)
            | (static_cast<unsigned int>(static_cast<unsigned char>(in[i+1])) << 8)
            | static_cast<unsigned int>(static_cast<unsigned char>(in[i+2]));
        out.append(alphabet[(v >> 18) & 0x3F]);
        out.append(alphabet[(v >> 12) & 0x3F]);
        out.append(alphabet[(v >> 6) & 0x3F]);
        out.append(alphabet[v & 0x3F]);
        i += 3;
    }
    if (i < n) {
        unsigned int v = static_cast<unsigned int>(static_cast<unsigned char>(in[i])) << 16;
        if (i + 1 < n) v |= static_cast<unsigned int>(static_cast<unsigned char>(in[i+1])) << 8;
        out.append(alphabet[(v >> 18) & 0x3F]);
        out.append(alphabet[(v >> 12) & 0x3F]);
        if (i + 1 < n) {
            out.append(alphabet[(v >> 6) & 0x3F]);
            if (!urlSafe) out.append('=');
        } else {
            if (!urlSafe) out.append("==", 2);
        }
    }
}

void binaryEncodeUU(const std::vector<std::byte> &in, webstrada::string &out) {
    size_t n = in.size();
    size_t pos = 0;
    while (pos < n) {
        size_t lineBytes = (n - pos > 45) ? 45 : (n - pos);
        out.append(static_cast<char>(32 + static_cast<int>(lineBytes)));
        size_t groups = (lineBytes + 2) / 3;
        size_t chars = 0;
        size_t end = pos + lineBytes;
        size_t i = pos;
        while (i < end) {
            unsigned char b[3] = {0, 0, 0};
            size_t got = 0;
            while (i < end && got < 3) b[got++] = static_cast<unsigned char>(in[i++]);
            out.append(static_cast<char>((b[0] >> 2) + 32));
            out.append(static_cast<char>((((b[0] & 0x3) << 4) | (b[1] >> 4)) + 32));
            chars += 2;
            if (got >= 2) {
                out.append(static_cast<char>((((b[1] & 0xF) << 2) | (b[2] >> 6)) + 32));
                chars++;
            }
            if (got >= 3) {
                out.append(static_cast<char>((b[2] & 0x3F) + 32));
                chars++;
            }
        }
        while (chars < groups * 4) {
            out.append(' ');
            chars++;
        }
        out.append('\n');
        pos += lineBytes;
    }
}

void variantToBytes(const cfvariant *v, const webstrada::string &encoding, std::vector<std::byte> &out) {
    if (!v) throw webstrada::exception("Function argument cannot be null");
    if (v->m_type == cfvariant::Binary && v->m_binary) {
        out = *v->m_binary;
    } else {
        cfml::stringToBytes(cfml::variantToString(*v), encoding, out);
    }
}

const EVP_MD *cryptoDigestByName(const webstrada::string &alg) {
    webstrada::string a = alg;
    a.toUpper();
    if (a.startWith("HMAC")) a = a.mid(4, a.length() - 4);
    webstrada::string b;
    for (int i = 0; i < a.length(); i++) {
        char c = a.at(i);
        if (c == '-' || c == '_') continue;
        b.append(c);
    }
    if (b.equals("MD5")) return EVP_md5();
    if (b.equals("SHA") || b.equals("SHA1")) return EVP_sha1();
    if (b.equals("SHA256")) return EVP_sha256();
    if (b.equals("SHA384")) return EVP_sha384();
    if (b.equals("SHA512")) return EVP_sha512();
    return nullptr;
}

webstrada::string uppercaseHex(const unsigned char *data, size_t len) {
    webstrada::string out;
    for (size_t i = 0; i < len; i++) {
        out.append(cfml::cryptoHexDigits[data[i] >> 4]);
        out.append(cfml::cryptoHexDigits[data[i] & 0xF]);
    }
    return out;
}

void loadCryptoProviders() {
    static bool loaded = false;
    if (!loaded) {
        OSSL_PROVIDER_load(nullptr, "legacy");
        OSSL_PROVIDER_load(nullptr, "default");
        loaded = true;
    }
}

void decodeEncryptKey(const webstrada::string &key, std::vector<std::byte> &out) {
    try {
        binaryDecodeBase64(key, out, false);
        if ((key.length() % 4) != 0) {
            out.clear();
            throw webstrada::exception("Invalid base64 length");
        }
    } catch (...) {
        throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: '' Can not decode string \"" + key + "\"..");
    }
}

bool stringEqualsNoCase(const webstrada::string &a, const webstrada::string &b) {
    return a.compareCaseInsensitive(b) == 0;
}

bool parseCipherAlgorithm(const webstrada::string &algStr, CipherAlg &alg) {
    webstrada::string a = algStr;
    a.toUpper();
    std::vector<webstrada::string> parts = a.split('/');
    const webstrada::string &base = parts[0];
    if (base.equals("CFMX_COMPAT")) {
        throw webstrada::exception(CFMX_COMPAT_ERROR);
    }
    if (base.equals("AES")) {
        alg.base = CipherAlg::CipherAES;
        alg.blockSize = 16;
    } else if (base.equals("DES")) {
        alg.base = CipherAlg::CipherDES;
        alg.blockSize = 8;
    } else if (base.equals("DESEDE") || base.equals("3DES") || base.equals("TRIPLEDES")) {
        alg.base = CipherAlg::CipherDESede;
        alg.blockSize = 8;
    } else if (base.equals("BLOWFISH")) {
        alg.base = CipherAlg::CipherBlowfish;
        alg.blockSize = 16;
    } else {
        return false;
    }
    if (parts.size() > 1) {
        alg.mode = parts[1];
        if (alg.mode.isEmpty()) alg.mode = "ECB";
    }
    if (parts.size() > 2) {
        webstrada::string pad = parts[2];
        pad.toUpper();
        if (pad.equals("NOPADDING")) alg.padding = false;
        else if (pad.equals("PKCS5PADDING") || pad.equals("PKCS7PADDING")) alg.padding = true;
        else {
            throw webstrada::exception("The " + algStr + " algorithm is not supported by the Security Provider you have chosen.");
        }
    }
    return true;
}

const EVP_CIPHER *cipherForAlg(const CipherAlg &alg, const std::vector<std::byte> &key, int &keyLen) {
    loadCryptoProviders();
    int klen = static_cast<int>(key.size());
    keyLen = klen;
    webstrada::string mode = alg.mode;
    mode.toUpper();
    bool ecb = mode.equals("ECB");
    bool cbc = mode.equals("CBC");
    bool cfb = mode.equals("CFB");
    bool ofb = mode.equals("OFB");
    bool ctr = mode.equals("CTR");
    switch (alg.base) {
        case CipherAlg::CipherAES:
            if (ecb) {
                if (klen == 16) return EVP_aes_128_ecb();
                if (klen == 24) return EVP_aes_192_ecb();
                if (klen == 32) return EVP_aes_256_ecb();
            } else if (cbc) {
                if (klen == 16) return EVP_aes_128_cbc();
                if (klen == 24) return EVP_aes_192_cbc();
                if (klen == 32) return EVP_aes_256_cbc();
            } else if (cfb) {
                if (klen == 16) return EVP_aes_128_cfb();
                if (klen == 24) return EVP_aes_192_cfb();
                if (klen == 32) return EVP_aes_256_cfb();
            } else if (ofb) {
                if (klen == 16) return EVP_aes_128_ofb();
                if (klen == 24) return EVP_aes_192_ofb();
                if (klen == 32) return EVP_aes_256_ofb();
            } else if (ctr) {
                if (klen == 16) return EVP_aes_128_ctr();
                if (klen == 24) return EVP_aes_192_ctr();
                if (klen == 32) return EVP_aes_256_ctr();
            }
            break;
        case CipherAlg::CipherDES:
            if (ecb && klen == 8) return EVP_des_ecb();
            if (cbc && klen == 8) return EVP_des_cbc();
            if (cfb && klen == 8) return EVP_des_cfb();
            if (ofb && klen == 8) return EVP_des_ofb();
            break;
        case CipherAlg::CipherDESede:
            if (ecb && klen == 24) return EVP_des_ede3_ecb();
            if (cbc && klen == 24) return EVP_des_ede3_cbc();
            if (cfb && klen == 24) return EVP_des_ede3_cfb();
            if (ofb && klen == 24) return EVP_des_ede3_ofb();
            break;
        case CipherAlg::CipherBlowfish:
            if (ecb) return EVP_bf_ecb();
            if (cbc) return EVP_bf_cbc();
            if (cfb) return EVP_bf_cfb();
            if (ofb) return EVP_bf_ofb();
            break;
        default:
            break;
    }
    return nullptr;
}

void cipherEncrypt(
    const std::vector<std::byte> &input,
    const CipherAlg &alg,
    const std::vector<std::byte> &key,
    const std::vector<std::byte> &iv,
    bool prependIv,
    std::vector<std::byte> &out)
{
    loadCryptoProviders();
    int keyLen = 0;
    const EVP_CIPHER *cipher = cipherForAlg(alg, key, keyLen);
    if (!cipher) {
        throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: Wrong key size: must be a valid key for the " + alg.mode + " algorithm");
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: Out of memory");
    int rc = EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr);
    if (rc == 1 && !alg.padding) rc = EVP_CIPHER_CTX_set_padding(ctx, 0);
    std::vector<std::byte> useIv = iv;
    if (rc == 1 && !stringEqualsNoCase(alg.mode, "ECB") && useIv.empty()) {
        // No IV given: generate a random IV and prepend it to the output.
        useIv.resize(alg.blockSize);
        if (RAND_bytes(reinterpret_cast<unsigned char*>(useIv.data()), alg.blockSize) != 1) {
            rc = 0;
        }
    }
    if (rc == 1 && !stringEqualsNoCase(alg.mode, "ECB") && static_cast<int>(useIv.size()) != alg.blockSize) {
        EVP_CIPHER_CTX_free(ctx);
        throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: Wrong IV length: must be " + webstrada::string::number(alg.blockSize) + " bytes long.");
    }
    if (rc == 1) {
        rc = EVP_EncryptInit_ex(ctx, nullptr, nullptr,
            reinterpret_cast<const unsigned char*>(key.data()),
            (stringEqualsNoCase(alg.mode, "ECB") || useIv.empty()) ? nullptr : reinterpret_cast<const unsigned char*>(useIv.data()));
    }
    std::vector<std::byte> result;
    result.resize(input.size() + static_cast<size_t>(alg.blockSize) + 16);
    int outLen = 0;
    int total = 0;
    if (rc == 1) {
        rc = EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(result.data()), &outLen,
            reinterpret_cast<const unsigned char*>(input.data()), static_cast<int>(input.size()));
        total = outLen;
    }
    if (rc == 1) {
        int finLen = 0;
        rc = EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(result.data()) + total, &finLen);
        total += finLen;
    }
    EVP_CIPHER_CTX_free(ctx);
    if (rc != 1) {
        throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: Encryption failed");
    }
    result.resize(total);
    if (prependIv && !useIv.empty()) {
        out.insert(out.end(), useIv.begin(), useIv.end());
    }
    out.insert(out.end(), result.begin(), result.end());
}

void cipherDecrypt(
    const std::vector<std::byte> &input,
    const CipherAlg &alg,
    const std::vector<std::byte> &key,
    const std::vector<std::byte> &iv,
    std::vector<std::byte> &out)
{
    loadCryptoProviders();
    int keyLen = 0;
    const EVP_CIPHER *cipher = cipherForAlg(alg, key, keyLen);
    if (!cipher) {
        throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: Wrong key size: must be a valid key for the algorithm");
    }
    std::vector<std::byte> data = input;
    std::vector<std::byte> useIv = iv;
    if (!stringEqualsNoCase(alg.mode, "ECB") && useIv.empty()) {
        // No IV given: the IV is the first block of the input.
        if (static_cast<int>(data.size()) < alg.blockSize) {
            out.clear();
            return;
        }
        useIv.assign(data.begin(), data.begin() + alg.blockSize);
        data.erase(data.begin(), data.begin() + alg.blockSize);
    }
    if (!stringEqualsNoCase(alg.mode, "ECB") && static_cast<int>(useIv.size()) != alg.blockSize) {
        throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: Wrong IV length: must be " + webstrada::string::number(alg.blockSize) + " bytes long.");
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: Out of memory");
    int rc = EVP_DecryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr);
    if (rc == 1 && !alg.padding) rc = EVP_CIPHER_CTX_set_padding(ctx, 0);
    if (rc == 1) {
        rc = EVP_DecryptInit_ex(ctx, nullptr, nullptr,
            reinterpret_cast<const unsigned char*>(key.data()),
            (stringEqualsNoCase(alg.mode, "ECB") || useIv.empty()) ? nullptr : reinterpret_cast<const unsigned char*>(useIv.data()));
    }
    std::vector<std::byte> result;
    result.resize(data.size() + static_cast<size_t>(alg.blockSize) + 16);
    int outLen = 0;
    int total = 0;
    if (rc == 1) {
        rc = EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(result.data()), &outLen,
            reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()));
        total = outLen;
    }
    if (rc == 1) {
        int finLen = 0;
        rc = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(result.data()) + total, &finLen);
        total += finLen;
    }
    EVP_CIPHER_CTX_free(ctx);
    if (rc != 1) {
        throw webstrada::exception("An error occurred while trying to encrypt or decrypt your input string: Given final block not properly padded. Such issues can arise if a bad key is used during decryption.");
    }
    result.resize(total);
    out.insert(out.end(), result.begin(), result.end());
}

// ---- from fn_file ----

void splitContentType(const std::string &ct, std::string &type, std::string &subtype) {
    size_t slash = ct.find('/');
    if (slash == std::string::npos) {
        type = ct;
        subtype.clear();
    } else {
        type = ct.substr(0, slash);
        subtype = ct.substr(slash + 1);
    }
    for (auto &c : type) c = std::tolower((unsigned char)c);
    for (auto &c : subtype) c = std::tolower((unsigned char)c);
}

void splitNameExt(const std::string &filename, std::string &name, std::string &ext) {
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos || dot == 0 || dot == filename.size() - 1) {
        name = filename;
        ext.clear();
    } else {
        name = filename.substr(0, dot);
        ext = filename.substr(dot + 1);
    }
}

std::string fileExtensionLower(const std::string &filename) {
    std::string ext;
    std::string name;
    splitNameExt(filename, name, ext);
    for (auto &c : ext) c = std::tolower((unsigned char)c);
    return ext;
}

std::string normalizeAllowItem(const webstrada::string &raw) {
    std::string item(raw.constData(), raw.length());
    while (!item.empty() && (item.front() == ' ' || item.front() == '\t')) item.erase(item.begin());
    while (!item.empty() && (item.back() == ' ' || item.back() == '\t')) item.pop_back();
    for (auto &c : item) c = std::tolower((unsigned char)c);
    return item;
}

bool mimeTypeMatches(const webstrada::UploadedFile &file, const webstrada::string &mimeList) {
    if (mimeList.isEmpty()) return true;

    std::string contentType = file.contentType;
    if (contentType.empty()) contentType = "application/octet-stream";
    for (auto &c : contentType) c = std::tolower((unsigned char)c);
    std::string ext = fileExtensionLower(file.filename);

    auto items = mimeList.split(',', false);
    for (const auto &itemRaw : items) {
        std::string item = normalizeAllowItem(itemRaw);
        if (item.empty()) continue;
        if (item == "*") return true;
        if (item[0] == '.') {
            if (!ext.empty() && item.substr(1) == ext) return true;
        } else {
            if (item == contentType) return true;
            // Allow a bare type (e.g. "text") to match "text/plain".
            if (contentType.size() > item.size() + 1 &&
                contentType.compare(0, item.size(), item) == 0 &&
                contentType[item.size()] == '/') return true;
        }
    }
    return false;
}

cfvariant *buildUploadStruct(const webstrada::UploadedFile &file,
                                    const std::string &serverDir,
                                    const std::string &serverFile,
                                    const std::string &attemptedServerFile,
                                    bool fileExisted,
                                    bool wasSaved,
                                    bool wasOverwritten,
                                    bool wasRenamed,
                                    int oldFileSize) {
    std::string clientName, clientExt;
    splitNameExt(file.filename, clientName, clientExt);
    std::string serverName, serverExt;
    splitNameExt(serverFile, serverName, serverExt);

    std::string ctType, ctSubtype;
    splitContentType(file.contentType.empty() ? "application/octet-stream" : file.contentType, ctType, ctSubtype);

    std::time_t t = std::time(nullptr);
    struct tm tm_local;
    localtime_r(&t, &tm_local);
    double days = tmToDays(tm_local);

    auto *ret = new cfvariant(cfvariant::Struct);
    ret->set("ATTEMPTEDSERVERFILE") = cfvariant(attemptedServerFile.c_str());
    ret->set("CLIENTDIRECTORY") = cfvariant("");
    ret->set("CLIENTFILE") = cfvariant(file.filename.c_str());
    ret->set("CLIENTFILEEXT") = cfvariant(clientExt.c_str());
    ret->set("CLIENTFILENAME") = cfvariant(clientName.c_str());
    ret->set("CONTENTSUBTYPE") = cfvariant(ctSubtype.c_str());
    ret->set("CONTENTTYPE") = cfvariant(ctType.c_str());

    cfvariant d(cfvariant::DateTime);
    d.m_double = days;
    ret->set("DATELASTACCESSED") = d;

    cfvariant b(cfvariant::Boolean);
    b.m_bool = fileExisted;
    ret->set("FILEEXISTED") = b;
    b.m_bool = false;
    ret->set("FILEWASAPPENDED") = b;
    b.m_bool = wasOverwritten;
    ret->set("FILEWASOVERWRITTEN") = b;
    b.m_bool = wasRenamed;
    ret->set("FILEWASRENAMED") = b;
    b.m_bool = wasSaved;
    ret->set("FILEWASSAVED") = b;

    ret->set("FILESIZE") = cfvariant(static_cast<int>(file.content.size()));
    ret->set("OLDFILESIZE") = cfvariant(oldFileSize);
    ret->set("SERVERDIRECTORY") = cfvariant(serverDir.empty() || serverDir.back() != '/' ? serverDir.c_str() : serverDir.substr(0, serverDir.size() - 1).c_str());
    ret->set("SERVERFILE") = cfvariant(serverFile.c_str());
    ret->set("SERVERFILEEXT") = cfvariant(serverExt.c_str());
    ret->set("SERVERFILENAME") = cfvariant(serverName.c_str());

    cfvariant d2(cfvariant::DateTime);
    d2.m_double = days;
    ret->set("TIMECREATED") = d2;
    ret->set("TIMELASTMODIFIED") = d2;

    // The upload result is returned through several callers (cf_fileupload,
    // cf_fileuploadall, the <cffile> tag); register it so every caller frees it
    // via the request cleanup (idempotent against caller-side registration).
    cf_register_temp(ret);
    return ret;
}

cfvariant *saveUploadedFile(const webstrada::UploadedFile &file,
                                   const std::string &serverDir,
                                   const webstrada::string &conflict,
                                   const std::string &initialServerFile) {
    std::string serverFile = initialServerFile.empty() ? file.filename : initialServerFile;
    std::string attemptedServerFile = serverFile;
    std::string fullPath = serverDir + serverFile;
    bool existed = std::filesystem::exists(fullPath);
    bool wasSaved = true, wasOverwritten = false, wasRenamed = false;

    if (existed) {
        if (conflict.isEmpty() || conflict.equals("skip")) {
            return buildUploadStruct(file, serverDir, serverFile, attemptedServerFile, true, false, false, false, static_cast<int>(file.content.size()));
        } else if (conflict.equals("overwrite")) {
            wasOverwritten = true;
        } else if (conflict.equals("makeunique")) {
            std::string name, ext;
            splitNameExt(serverFile, name, ext);
            int counter = 1;
            std::string candidate;
            do {
                candidate = name + std::to_string(counter) + (ext.empty() ? "" : "." + ext);
                counter++;
            } while (std::filesystem::exists(serverDir + candidate));
            serverFile = candidate;
            fullPath = serverDir + serverFile;
            wasRenamed = true;
        } else {
            throw webstrada::exception("FileUpload: A file with the same name already exists in the destination directory. Use onConflict=\"overwrite\", \"skip\" or \"makeunique\" to control this behavior.");
        }
    }

    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        throw webstrada::exception("FileUpload: Failed to write uploaded file: " + string(fullPath.c_str()));
    }
    const char *data = reinterpret_cast<const char*>(file.content.data());
    size_t written = 0;
    while (written < file.content.size()) {
        ssize_t n = write(fd, data + written, file.content.size() - written);
        if (n < 0) {
            close(fd);
            throw webstrada::exception("FileUpload: Failed to write uploaded file: " + string(fullPath.c_str()));
        }
        written += static_cast<size_t>(n);
    }
    close(fd);

    return buildUploadStruct(file, serverDir, serverFile, attemptedServerFile, existed, wasSaved, wasOverwritten, wasRenamed, static_cast<int>(file.content.size()));
}

void resolveUploadDestination(const webstrada::string &destStr, const char *funcName,
                                     std::string &serverDir, bool &dirMode,
                                     std::string &fileBase) {
    std::filesystem::path destPath(destStr.constData());
    if (!destPath.is_absolute()) {
        destPath = std::filesystem::temp_directory_path() / destPath;
    }

    dirMode = std::filesystem::is_directory(destPath);
    std::filesystem::path parent = dirMode ? destPath : destPath.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
        throw webstrada::exception(string(funcName) + ": Failed to create destination directory: " + destStr);
    }

    serverDir = parent.string();
    if (!serverDir.empty() && serverDir.back() != '/') serverDir += '/';
    fileBase = dirMode ? std::string() : destPath.filename().string();
}

// ---- from fn_display ----

const cfml::LocaleInfo *currentLocale()
{
    return g_currentLocale ? g_currentLocale : cfml::locale_default();
}

const char *currentLocaleStr()
{
    if (g_currentLocale && !g_currentLocaleStr.isEmpty()) return g_currentLocaleStr.constData();
    return "en_US";
}

void locale_reset()
{
    g_currentLocale = nullptr;
    g_currentLocaleStr.clear();
}

const cfml::LocaleInfo *resolveLocale(const cfvariant *localeArg)
{
    if (localeArg && localeArg->m_type != cfvariant::Null) {
        string name = const_cast<cfvariant*>(localeArg)->toString();
        // CF's CFLocaleMgr registers the empty string as an alias of the
        // English (US) default locale (verified on CF 2025: an empty locale
        // argument formats/parses as en_US); unknown names still throw.
        if (name.length() == 0) return cfml::locale_default();
        const cfml::LocaleInfo *loc = cfml::locale_find(name.constData());
        if (!loc) {
            throw webstrada::exception("The locale, " + name + ", cannot be found.");
        }
        return loc;
    }
    return currentLocale();
}

std::string groupDigits(const std::string &digits, const char *groupSep)
{
    if (digits.empty()) return digits;
    std::string out;
    int n = static_cast<int>(digits.length());
    for (int i = 0; i < n; i++) {
        if (i > 0 && (n - i) % 3 == 0) out += groupSep;
        out += digits[i];
    }
    return out;
}

bool parseNumberWithLocale(const string &s, const cfml::LocaleInfo *loc, double &out)
{
    std::string t = s.constData();
    std::string dec(loc->numDecSep);
    std::string grp(loc->numGroupSep);
    std::string cleaned;
    for (size_t i = 0; i < t.length(); ) {
        if (t.compare(i, dec.length(), dec) == 0) {
            cleaned += '.';
            i += dec.length();
        } else if (t.compare(i, grp.length(), grp) == 0 || t[i] == ' ') {
            i += (t[i] == ' ') ? 1 : grp.length();
        } else {
            cleaned += t[i];
            i++;
        }
    }
    char *end = nullptr;
    double v = strtod(cleaned.c_str(), &end);
    if (end == cleaned.c_str() || (end && *end != '\0')) return false;
    out = v;
    return true;
}

long long roundHalfEven(double v)
{
    double lo = std::floor(v);
    double frac = v - lo;
    long long ilo = static_cast<long long>(lo);
    if (frac > 0.5) return ilo + 1;
    if (frac < 0.5) return ilo;
    return (ilo % 2 == 0) ? ilo : ilo + 1;
}

std::string formatCurrencyPattern(double absNum, const char *pattern, const cfml::LocaleInfo *loc)
{
    std::string pat(pattern ? pattern : "");
    int decimals = loc->curDecimals;
    long long scale = 1;
    for (int i = 0; i < decimals; i++) scale *= 10;
    long long rounded = roundHalfEven(absNum * scale);
    long long ip = rounded / scale;
    long long fp = rounded % scale;
    std::string digits = std::to_string(ip);
    std::string body = groupDigits(digits, loc->curGroupSep);
    if (decimals > 0) {
        char fb[32];
        std::snprintf(fb, sizeof(fb), "%0*lld", decimals, fp);
        body += loc->curDecSep;
        body += fb;
    }

    // Build prefix/suffix around the digit run: ¤ (UTF-8 \xC2\xA4) becomes the
    // symbol, ',' the grouping separator, '.' the decimal separator.
    std::string sym(loc->curSymbol);
    std::string group(loc->curGroupSep);
    std::string dec(loc->curDecSep);
    auto render = [&](const std::string &seg) -> std::string {
        std::string o;
        for (size_t i = 0; i < seg.length(); i++) {
            unsigned char c = static_cast<unsigned char>(seg[i]);
            if (c == 0xC2 && i + 1 < seg.length() && static_cast<unsigned char>(seg[i + 1]) == 0xA4) { o += sym; i++; continue; }
            if (c == ',') { o += group; continue; }
            if (c == '.') { o += dec; continue; }
            o += c;
        }
        return o;
    };
    size_t ds = pat.find_first_of("#09");
    size_t de = pat.find_last_of("#09");
    if (ds != std::string::npos && de != std::string::npos && de >= ds) {
        std::string prefix = render(pat.substr(0, ds));
        std::string suffix = render(pat.substr(de + 1));
        return prefix + body + suffix;
    }
    return render(pat);
}

std::string stripCurrencySymbol(const std::string &in, const cfml::LocaleInfo *loc)
{
    std::string s = in;
    std::string sym(loc->curSymbol);
    size_t pos = s.find(sym);
    if (pos == std::string::npos) return s;
    s.erase(pos, sym.length());
    if (pos < s.length() && s[pos] == ' ') s.erase(pos, 1);
    else if (pos > 0 && s[pos - 1] == ' ') s.erase(pos - 1, 1);
    return s;
}

std::string formatCurrency(double num, const char *type, const cfml::LocaleInfo *loc)
{
    bool negative = num < 0.0;
    double absv = std::abs(num);
    std::string local = formatCurrencyPattern(absv, negative ? loc->curNegPattern : loc->curPattern, loc);

    std::string t = type ? type : "local";
    std::string lower = t;
    for (auto &c : lower) c = (char)tolower((unsigned char)c);
    if (lower == "local") return local;

    // international / none: strip the local symbol (and adjacent space).
    std::string stripped = stripCurrencySymbol(local, loc);
    if (lower == "none") return stripped;
    // international
    return std::string(loc->curIntl) + stripped;
}

double lsNumberValue(const cfvariant *num, const char *func)
{
    if (!num) throw webstrada::exception(string(func) + " requires at least 1 argument");
    return getDoubleValue(*num);
}

// ---- from fn_misc ----

string formatShortestDouble(double value) {
    if (std::isnan(value)) return string("\xEF\xBF\xBD");
    if (std::isinf(value)) return string(value > 0 ? "Infinity" : "-Infinity");
    if (value == 0.0) return string("0"); // CF renders computed -0.0 as "0"
    char buf[64];
    auto res = std::to_chars(buf, buf + sizeof(buf), value, std::chars_format::general);
    if (res.ec == std::errc()) {
        return string(buf, static_cast<size_t>(res.ptr - buf));
    }
    return string::number(value);
}

// ---- from fn_json ----

std::string cfJsonDouble(double d)
{
    // Shortest round-trip digit string via the %.15g/%.16g/%.17g ladder.
    char buf[64];
    int prec = 15;
    for (; prec <= 17; prec++) {
        std::snprintf(buf, sizeof(buf), "%.*g", prec, d);
        if (std::strtod(buf, nullptr) == d) break;
    }
    std::string s(buf);

    bool neg = false;
    if (!s.empty() && s[0] == '-') { neg = true; s = s.substr(1); }

    size_t e = s.find_first_of("eE");
    std::string mant = (e == std::string::npos) ? s : s.substr(0, e);
    int exp10 = (e == std::string::npos) ? 0 : std::atoi(s.c_str() + e + 1);

    std::string digits;
    size_t dot = mant.find('.');
    if (dot == std::string::npos) {
        digits = mant;
        exp10 += (int)digits.size() - 1;
    } else {
        digits = mant.substr(0, dot) + mant.substr(dot + 1);
        exp10 += (int)dot - 1;
    }
    size_t lead = digits.find_first_not_of('0');
    if (lead == std::string::npos) {
        digits = "0";
        exp10 = 0;
    } else if (lead > 0) {
        digits = digits.substr(lead);
        exp10 -= (int)lead;
    }

    // A shortest round-trip representation never ends in a zero digit; %.Ng can
    // produce plain forms like "1000000000" for 1e9, so drop trailing zeros.
    size_t trail = digits.find_last_not_of('0');
    if (trail == std::string::npos) {
        digits = "0";
        exp10 = 0;
    } else {
        digits = digits.substr(0, trail + 1);
    }

    std::string out;
    if (exp10 >= 7 || exp10 < -3) {
        // Scientific (Java Double.toString): one digit before the point,
        // at least one fractional digit, exponent without padding/plus.
        std::string frac = (digits.size() > 1) ? digits.substr(1) : std::string("0");
        out = digits.substr(0, 1) + "." + frac + "E" + std::to_string(exp10);
    } else if (exp10 >= 0) {
        if ((int)digits.size() > exp10 + 1) {
            out = digits.substr(0, exp10 + 1) + "." + digits.substr(exp10 + 1);
        } else {
            out = digits + std::string(exp10 + 1 - (int)digits.size(), '0');
        }
    } else {
        out = "0." + std::string(-exp10 - 1, '0') + digits;
    }
    return neg ? "-" + out : out;
}

unsigned int javaStringHash(const char *s)
{
    unsigned int h = 0;
    if (!s) return 0;
    for (; *s; s++) h = h * 31 + (unsigned char)*s;
    return h;
}

int javaHashMapBucket(const char *key, int capacity)
{
    std::string u;
    if (key) {
        u = key;
        for (auto &c : u) { if (c >= 'a' && c <= 'z') c = (char)(c - 32); }
    }
    unsigned int h = javaStringHash(u.c_str());
    h ^= h >> 16;
    return (int)(h & (unsigned int)(capacity - 1));
}

int javaHashMapCapacity(size_t size)
{
    int cap = 16;
    while (size > (size_t)(cap * 3 / 4)) cap *= 2;
    return cap;
}

static webstrada::cfvariant coercePropertyDefault(const std::string &typeStr, const std::string &defaultText)
{
    using webstrada::cfvariant;
    std::string t = typeStr;
    for (auto &c : t) c = (char)tolower((unsigned char)c);
    std::string def = defaultText;
    size_t ls = def.find_first_not_of(" \t\r\n");
    size_t le = def.find_last_not_of(" \t\r\n");
    if (ls == std::string::npos) def = "";
    else def = def.substr(ls, le - ls + 1);
    if (def.size() >= 2 &&
        ((def.front() == '"' && def.back() == '"') ||
         (def.front() == '\'' && def.back() == '\''))) {
        def = def.substr(1, def.size() - 2);
    }

    if (t == "numeric" || t == "number" || t == "integer" || t == "int" || t == "long" || t == "double" || t == "float") {
        try {
            size_t idx = 0;
            double d = std::stod(def, &idx);
            if (idx == def.size()) {
                if (def.find('.') != std::string::npos || def.find('e') != std::string::npos || def.find('E') != std::string::npos) {
                    cfvariant v(cfvariant::Float);
                    v.m_double = d;
                    return v;
                } else {
                    long long ll = std::stoll(def);
                    if (ll >= INT32_MIN && ll <= INT32_MAX) {
                        return cfvariant(static_cast<int>(ll));
                    } else {
                        cfvariant v(cfvariant::Long);
                        v.m_long = ll;
                        return v;
                    }
                }
            }
        } catch (...) {}
    } else if (t == "boolean" || t == "bool") {
        std::string lowDef = def;
        for (auto &c : lowDef) c = (char)tolower((unsigned char)c);
        cfvariant v(cfvariant::Boolean);
        if (lowDef == "true" || lowDef == "yes" || lowDef == "1") {
            v.m_bool = true;
            return v;
        } else if (lowDef == "false" || lowDef == "no" || lowDef == "0") {
            v.m_bool = false;
            return v;
        }
    }
    return cfvariant(def.c_str());
}

std::vector<webstrada::string> getSortedComponentKeys(const cfvariant *compVal, bool includeProperties, bool includeThisScope, bool includeMethods)
{
    using webstrada::ComponentInstance;
    using webstrada::ComponentInfo;
    std::vector<webstrada::string> result;
    if (!compVal || compVal->m_type != cfvariant::Component || !compVal->m_component) {
        return result;
    }
    ComponentInstance *inst = compVal->m_component;
    if (!inst->info) return result;

    std::vector<std::pair<webstrada::string, size_t>> rawKeys;
    std::set<webstrada::string> seenUpper;
    size_t insIdx = 0;

    std::vector<ComponentInfo*> chain;
    for (ComponentInfo *i = inst->info; i; i = i->parent) chain.push_back(i);

    if (includeThisScope && inst->thisScope && inst->thisScope->m_struct) {
        std::vector<webstrada::string> thisKeys;
        if (inst->thisScope->m_structInsertOrder) {
            for (const auto &k : *inst->thisScope->m_structInsertOrder) {
                if (inst->thisScope->m_struct->count(k)) {
                    thisKeys.push_back(k);
                }
            }
        }
        for (const auto &kv : *inst->thisScope->m_struct) {
            bool found = false;
            for (const auto &k : thisKeys) {
                if (k.equals(kv.first)) { found = true; break; }
            }
            if (!found) thisKeys.push_back(kv.first);
        }
        for (const auto &k : thisKeys) {
            auto it = inst->thisScope->m_struct->find(k);
            if (it != inst->thisScope->m_struct->end() && it->second.m_type == cfvariant::Function) continue;
            webstrada::string u = k;
            u.toUpper();
            if (seenUpper.insert(u).second) {
                rawKeys.push_back({k, insIdx++});
            }
        }
    }

    if (includeProperties) {
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            for (const auto &p : (*it)->properties) {
                if (p.defaultText.empty()) continue;
                webstrada::string u(p.name.c_str());
                u.toUpper();
                if (seenUpper.insert(u).second) {
                    rawKeys.push_back({webstrada::string(p.name.c_str()), insIdx++});
                }
            }
        }
    }

    if (includeMethods) {
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            for (const auto &m : (*it)->methods) {
                std::string acc = m.access;
                for (auto &c : acc) c = (char)tolower((unsigned char)c);
                if (acc.empty() || acc == "public" || acc == "package" || acc == "remote") {
                    webstrada::string mName(m.declaredName.empty() ? m.name.c_str() : m.declaredName.c_str());
                    webstrada::string u = mName;
                    u.toUpper();
                    if (seenUpper.insert(u).second) {
                        rawKeys.push_back({mName, insIdx++});
                    }
                }
            }
        }
    }

    int cap = javaHashMapCapacity(rawKeys.size());
    std::stable_sort(rawKeys.begin(), rawKeys.end(), [&](const std::pair<webstrada::string, size_t> &a, const std::pair<webstrada::string, size_t> &b) {
        int ba = javaHashMapBucket(a.first.constData(), cap);
        int bb = javaHashMapBucket(b.first.constData(), cap);
        if (ba != bb) return ba < bb;
        return a.second < b.second;
    });

    for (const auto &p : rawKeys) {
        result.push_back(p.first);
    }
    return result;
}

// Frees a json_object on any exception path (json-c objects created for a
// container are otherwise leaked when a nested element's serialization throws,
// e.g. the circular-reference error).
struct JsonObjGuard {
    json_object *o;
    JsonObjGuard(json_object *obj) : o(obj) {}
    ~JsonObjGuard() { if (o) json_object_put(o); }
    json_object *release() { json_object *r = o; o = nullptr; return r; }
};

json_object *serialize_json_value(const cfvariant &val, const string &queryFormat, std::set<const void*> &visited) {
    (void)queryFormat;
    switch (val.m_type) {
        case cfvariant::Null:
            return json_object_new_string("");
        case cfvariant::Boolean:
            return json_object_new_boolean(val.m_bool);
        case cfvariant::Number:
            return json_object_new_int64(val.m_int);
        case cfvariant::Long:
            return json_object_new_int64(val.m_long);
        case cfvariant::Float:
            // Literal floats serialize with their original text (SerializeJSON([8.0]) -> [8.0]);
            // computed doubles use CF-style shortest-round-trip rendering.
            if (val.m_literalText) {
                return json_object_new_double_s(val.m_double, val.m_literalText->constData());
            }
            if (std::isfinite(val.m_double)) {
                std::string s = cfJsonDouble(val.m_double);
                return json_object_new_double_s(val.m_double, s.c_str());
            }
            // NaN / Inf: json-c renders the (non-JSON) value as null, matching
            // CF's SerializeJSON of non-finite numbers.
            return json_object_new_double(val.m_double);
        case cfvariant::String: {
            string s = const_cast<cfvariant&>(val).toString();
            const char *d = s.constData();
            return json_object_new_string(d ? d : "");
        }
        case cfvariant::DateTime: {
            // CF serializes dates as "January, 01 2020 00:00:00".
            cfvariant d(val);
            struct tm tmv = daysToTm(d.m_double);
            static const char *jsonMonths[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%s, %02d %04d %02d:%02d:%02d",
                jsonMonths[tmv.tm_mon], tmv.tm_mday, tmv.tm_year + 1900,
                tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
            return json_object_new_string(buf);
        }
        case cfvariant::Array: {
            // A column reference serializes as its scalar first cell (CF:
            // SerializeJSON(x) for x = q["a"] is "x", and SerializeJSON(q.a)
            // is "x"); only a freshly materialized bracket temp
            // (SerializeJSON(q["a"])) serializes as a real array.
            if (!isCfArray(&val)) {
                cfvariant first = queryColumnFirstCell(&val);
                return serialize_json_value(first, queryFormat, visited);
            }
            json_object *arr = json_object_new_array();
            JsonObjGuard arrGuard(arr);
            const void *akey = val.m_array;
            if (akey && !visited.insert(akey).second) {
                throw webstrada::exception("Unable to serialize a circular reference.");
            }
            for (size_t i = 0; i < val.m_array->size(); i++) {
                json_object *elem = serialize_json_value((*val.m_array)[i], queryFormat, visited);
                json_object_array_add(arr, elem);
            }
            if (akey) visited.erase(akey);
            return arrGuard.release();
        }
        case cfvariant::Struct: {
            json_object *obj = json_object_new_object();
            JsonObjGuard objGuard(obj);
            const void *skey = val.m_structData;
            if (skey && !visited.insert(skey).second) {
                throw webstrada::exception("Unable to serialize a circular reference.");
            }
            // ColdFusion's plain struct is a java.util.HashMap; SerializeJSON
            // emits keys in that map's iteration order (hash bucket, then
            // insertion order within the bucket). Replicate it here. Structs
            // that wrap live Java objects (ImageInfo's colormodel for
            // PackedColorModel) iterate as a LinkedHashMap, i.e. pure
            // insertion order, which CF preserves verbatim.
            std::vector<std::string> keys;
            keys.reserve(val.m_struct->size());
            for (auto &kv : *val.m_struct) keys.push_back(kv.first.constData());

            if (val.m_serializeInsertOrder) {
                std::vector<std::string> ordered;
                ordered.reserve(val.m_struct->size());
                if (val.m_structInsertOrder) {
                    for (auto &k : *val.m_structInsertOrder) {
                        const char *c = k.constData();
                        if (c && val.m_struct->count(c) && std::find(ordered.begin(), ordered.end(), c) == ordered.end())
                            ordered.push_back(c);
                    }
                }
                for (auto &kv : *val.m_struct) {
                    const char *c = kv.first.constData();
                    if (std::find(ordered.begin(), ordered.end(), c) == ordered.end())
                        ordered.push_back(c);
                }
                for (auto &k : ordered) {
                    json_object *v = serialize_json_value(val.m_struct->at(k.c_str()), queryFormat, visited);
                    json_object_object_add(obj, k.c_str(), v);
                }
                if (skey) visited.erase(skey);
                return objGuard.release();
            }

            std::map<std::string, size_t> insOrder;
            if (val.m_structInsertOrder) {
                for (size_t i = 0; i < val.m_structInsertOrder->size(); i++) {
                    const char *k = val.m_structInsertOrder->at(i).constData();
                    if (k) insOrder.emplace(k, i);
                }
            }

            int cap = javaHashMapCapacity(val.m_struct->size());
            std::stable_sort(keys.begin(), keys.end(), [&](const std::string &a, const std::string &b) {
                int ba = javaHashMapBucket(a.c_str(), cap);
                int bb = javaHashMapBucket(b.c_str(), cap);
                if (ba != bb) return ba < bb;
                auto ia = insOrder.find(a);
                auto ib = insOrder.find(b);
                size_t sa = (ia != insOrder.end()) ? ia->second : (size_t)-1;
                size_t sb = (ib != insOrder.end()) ? ib->second : (size_t)-1;
                if (sa != sb) return sa < sb;
                return a < b;
            });

            for (auto &k : keys) {
                json_object *v = serialize_json_value(val.m_struct->at(k.c_str()), queryFormat, visited);
                json_object_object_add(obj, k.c_str(), v);
            }
            if (skey) visited.erase(skey);
            return objGuard.release();
        }
        case cfvariant::Component: {
            json_object *obj = json_object_new_object();
            JsonObjGuard objGuard(obj);
            ComponentInstance *inst = val.m_component;
            if (!inst) return objGuard.release();
            const void *ckey = inst;
            if (ckey && !visited.insert(ckey).second) {
                throw webstrada::exception("Unable to serialize a circular reference.");
            }

            std::vector<webstrada::string> sortedKeys = getSortedComponentKeys(&val, true, true, false);
            for (const auto &key : sortedKeys) {
                webstrada::string pKey = key;
                pKey.toUpper();

                const webstrada::cfvariant *valPtr = nullptr;
                webstrada::string actualKey = key;
                if (inst->thisScope && inst->thisScope->m_struct) {
                    for (const auto &kv : *inst->thisScope->m_struct) {
                        if (kv.first.equals(pKey) && kv.second.m_type != cfvariant::Function) {
                            valPtr = &kv.second;
                            actualKey = kv.first;
                            break;
                        }
                    }
                }

                if (valPtr) {
                    json_object *v = serialize_json_value(*valPtr, queryFormat, visited);
                    json_object_object_add(obj, actualKey.constData(), v);
                } else {
                    for (ComponentInfo *ci = inst->info; ci; ci = ci->parent) {
                        bool found = false;
                        for (const auto &p : ci->properties) {
                            if (pKey.compareCaseInsensitive(p.name.c_str()) == 0) {
                                cfvariant coerced = coercePropertyDefault(p.type, p.defaultText);
                                json_object *v = serialize_json_value(coerced, queryFormat, visited);
                                json_object_object_add(obj, key.constData(), v);
                                found = true;
                                break;
                            }
                        }
                        if (found) break;
                    }
                }
            }

            if (ckey) visited.erase(ckey);
            return objGuard.release();
        }
        case cfvariant::Query: {
            // CF serializes a query as {"COLUMNS":["A","B"],"DATA":[[...],...]}.
            // COLUMNS preserves the original (upper-cased) column order; DATA is
            // row-major with null for missing cells. Verified against CF 2021.
            const QueryData *qd = val.m_query;
            json_object *obj = json_object_new_object();
            JsonObjGuard objGuard(obj);
            if (qd) {
                const void *qkey = qd;
                if (qkey && !visited.insert(qkey).second) {
                    throw webstrada::exception("Unable to serialize a circular reference.");
                }
                json_object *cols = json_object_new_array();
                for (auto &col : qd->columns) {
                    string cn = col.name;
                    cn.toUpper();
                    json_object_array_add(cols, json_object_new_string(cn.constData()));
                }
                json_object_object_add(obj, "COLUMNS", cols);

                json_object *data = json_object_new_array();
                int rows = qd->rowCount();
                for (int r = 0; r < rows; r++) {
                    json_object *row = json_object_new_array();
                    for (size_t c = 0; c < qd->columns.size(); c++) {
                        const cfvariant &cell = qd->columns[c].values[r];
                        if (cell.m_type == cfvariant::Null) {
                            json_object_array_add(row, json_object_new_null());
                        } else {
                            json_object_array_add(row, serialize_json_value(cell, queryFormat, visited));
                        }
                    }
                    json_object_array_add(data, row);
                }
                json_object_object_add(obj, "DATA", data);
                visited.erase(qkey);
            } else {
                json_object_object_add(obj, "COLUMNS", json_object_new_array());
                json_object_object_add(obj, "DATA", json_object_new_array());
            }
            return objGuard.release();
        }
        default:
            return json_object_new_string("null");
    }
}

cfvariant deserialize_json_value(json_object *obj, bool strictMapping, bool literalBooleans) {
    if (!obj) {
        cfvariant ret(cfvariant::Null);
        return ret;
    }
    switch (json_object_get_type(obj)) {
        case json_type_null: {
            cfvariant ret(cfvariant::Null);
            return ret;
        }
        case json_type_boolean: {
            cfvariant ret(cfvariant::Boolean);
            ret.m_bool = json_object_get_boolean(obj);
            ret.m_boolLiteral = literalBooleans;
            return ret;
        }
        case json_type_int: {
            cfvariant ret(cfvariant::Number);
            int64_t i = json_object_get_int64(obj);
            if (i >= -2147483648LL && i <= 2147483647LL) {
                ret.m_int = static_cast<int>(i);
            } else {
                // CF returns a Long for int64 JSON values (DeserializeJSON
                // "2147483648" -> 2147483648, "9007199254740993" -> exact).
                ret.set_type(cfvariant::Long);
                ret.m_long = i;
            }
            return ret;
        }
        case json_type_double: {
            cfvariant ret(cfvariant::Float);
            ret.m_double = json_object_get_double(obj);
            return ret;
        }
        case json_type_string: {
            const char *s = json_object_get_string(obj);
            cfvariant ret(s ? s : "");
            return ret;
        }
        case json_type_array: {
            (void)strictMapping;
            cfvariant ret(cfvariant::Array);
            int len = json_object_array_length(obj);
            for (int i = 0; i < len; i++) {
                json_object *elem = json_object_array_get_idx(obj, i);
                ret.insert(deserialize_json_value(elem, strictMapping, literalBooleans));
            }
            return ret;
        }
        case json_type_object: {
            cfvariant ret(cfvariant::Struct);
            json_object_object_foreach(obj, key, val) {
                ret.structSet(string(key), deserialize_json_value(val, strictMapping, literalBooleans));
            }
            return ret;
        }
    }
    cfvariant ret(cfvariant::Null);
    return ret;
}

std::string scope_json_serialize(const cfvariant &data)
{
    g_serializeVisited.clear();
    json_object *obj = serialize_json_value(data, string("row"), g_serializeVisited);
    const char *s = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
    std::string out = s ? s : "";
    json_object_put(obj);
    return out;
}

bool scope_json_deserialize(const std::string &text, cfvariant &out)
{
    if (text.empty()) return false;
    json_object *obj = json_tokener_parse(text.c_str());
    if (!obj) return false;
    bool ok = json_object_get_type(obj) != json_type_null;
    if (ok) out = deserialize_json_value(obj, true);
    json_object_put(obj);
    return ok;
}

// ---- from fn_dump ----

void cfdump_reset_page()
{
    g_cfdump_style_emitted = false;
    g_cfdump_abort_pending = false;
    g_cfdump_style_cache.clear();
    g_cfdump_udf_first_space = true;
    g_cfdump_udf_seen_depths.clear();
}

std::string formatCfdumpFloat(double d)
{
    if (std::isinf(d)) return d > 0 ? "1.#INF" : "-1.#INF";
    if (std::isnan(d)) return "\xEF\xBF\xBD";
    if (d == 0.0) return "0"; // CF renders computed -0.0 as "0"
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.12g", d);
    std::string s(buf);
    size_t e = s.find_first_of("eE");
    if (e != std::string::npos) {
        std::string mant = s.substr(0, e);
        int exp = std::atoi(s.c_str() + e + 1);
        char eb[32];
        std::snprintf(eb, sizeof(eb), "%+04d", exp);
        s = mant + "E" + eb;
    }
    return s;
}

void cfdump(string &out, const cfvariant &var)
{
    cfvariant *dumpResult = cf_writedump(&var, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    cf_emit_writedump(&out, dumpResult);
}

// ---- from fn_image ----

std::string toStdString(const cfvariant *v)
{
    if (!v) return "";
    webstrada::string tmp = const_cast<cfvariant*>(v)->toString();
    const char *d = tmp.constData();
    return d ? d : "";
}

const char *randDefaultAlgorithm() { return "SHA1PRNG"; }

// CF's SecureRandomGenerator.getRandomNumberGenerator() validates the algorithm
// name against the JCA providers (case-insensitive, no trimming) only when the
// per-thread generator cache is empty. Recognized names on CF 2025:
// SHA1PRNG, NativePRNG, NativePRNGNonBlocking, NativePRNGBlocking, DRBG,
// Windows-PRNG.
bool randAlgorithmValid(const std::string &algo)
{
    std::string up = algo;
    for (auto &c : up) c = static_cast<char>(toupper((unsigned char)c));
    static const std::vector<std::string> known = {
        "SHA1PRNG", "NATIVEPRNG", "NATIVEPRNGNONBLOCKING",
        "NATIVEPRNGBLOCKING", "DRBG", "WINDOWS-PRNG"};
    for (const auto &k : known) if (up == k) return true;
    return false;
}

void randAlgorithmValidate(const std::string &algo)
{
    if (!randAlgorithmValid(algo)) {
        throw exception(string(("The " + algo + " algorithm is not supported by the Security Provider you have chosen.").c_str()));
    }
}

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

double toDouble(const cfvariant *v)
{
    if (!v) throw exception("Parameter validation error: Expected a numeric value but received null.");
    switch (v->m_type) {
    case cfvariant::Number: return (double)v->m_int;
    case cfvariant::Long:   return (double)v->m_long;
    case cfvariant::Float:  return v->m_double;
    case cfvariant::Boolean: return v->m_bool ? 1.0 : 0.0;
    default: break;
    }
    webstrada::string tmp = const_cast<cfvariant*>(v)->toString();
    const char *str = tmp.constData();
    if (!str) throw exception("Parameter validation error: Expected a numeric value but received empty/null.");
    const char *p = str;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) throw exception("Parameter validation error: Expected a numeric value but received an empty string.");
    char *end = nullptr;
    double d = strtod(p, &end);
    while (end && *end && isspace((unsigned char)*end)) end++;
    if (end == p || (end && *end)) {
        throw exception("Parameter validation error: The value cannot be converted to a number.");
    }
    return d;
}

int toInt(const cfvariant *v) { return (int)toDouble(v); }

bool toBool(const cfvariant *v)
{
    if (!v) return false;
    if (v->m_type == cfvariant::Boolean) return v->m_bool;
    std::string s = toLower(toStdString(v));
    // trim
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    s = (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
    if (s.empty() || s == "false" || s == "no" || s == "off" || s == "0" || s == "null") return false;
    return true;
}

void imageThrow(const char *type, const std::string &message, const std::string &detail)
{
    throw exception(string(type), string(message.c_str()), string(detail.c_str()));
}

std::vector<std::byte> readFileBytes(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw exception("The system cannot find the file specified.");
    std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<std::byte> out;
    out.reserve(data.size());
    for (char c : data) out.push_back((std::byte)c);
    return out;
}

std::string resolveSourcePath(const std::string &path)
{
    char buf[4096];
    if (path.empty() || path[0] == '/') return path;
    const char *res = realpath(path.c_str(), buf);
    return res ? std::string(res) : path;
}

void writeFileBytes(const std::string &path, const std::vector<std::byte> &data, bool overwrite)
{
    if (!overwrite) {
        std::ifstream probe(path, std::ios::binary);
        if (probe.good()) {
            std::string absPath = resolveSourcePath(path);
            throw exception(string("Application"),
                            string(("The file " + absPath + " already exists.").c_str()),
                            string("Specify a new destination file or specify overwrite = \"true\"."));
        }
    }
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) throw exception(string(("Unable to write image file: " + path).c_str()));
    f.write((const char*)data.data(), (std::streamsize)data.size());
    f.close();
}

std::string fileExt(const std::string &path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot + 1);
    return toLower(ext);
}

ImageData *imageAlloc(int w, int h)
{
    auto *img = new ImageData;
    img->width = w;
    img->height = h;
    img->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    return img;
}

void surfaceRGBA(cairo_surface_t *sf, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
    const uint32_t *data = (const uint32_t*)cairo_image_surface_get_data(sf);
    int stride = cairo_image_surface_get_stride(sf);
    uint32_t p = data[y * stride / 4 + x];
    uint8_t aa = (uint8_t)((p >> 24) & 0xFF);
    uint8_t rr = (uint8_t)((p >> 16) & 0xFF);
    uint8_t gg = (uint8_t)((p >> 8) & 0xFF);
    uint8_t bb = (uint8_t)(p & 0xFF);
    if (aa && aa != 255) {
        rr = (uint8_t)((rr * 255) / aa);
        gg = (uint8_t)((gg * 255) / aa);
        bb = (uint8_t)((bb * 255) / aa);
    }
    *r = rr; *g = gg; *b = bb; *a = aa;
}

void surfaceSetRGBA(cairo_surface_t *sf, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    uint32_t *data = (uint32_t*)cairo_image_surface_get_data(sf);
    int stride = cairo_image_surface_get_stride(sf);
    uint8_t pr = (uint8_t)((r * a + 127) / 255);
    uint8_t pg = (uint8_t)((g * a + 127) / 255);
    uint8_t pb = (uint8_t)((b * a + 127) / 255);
    data[y * stride / 4 + x] = ((uint32_t)a << 24) | ((uint32_t)pr << 16) | ((uint32_t)pg << 8) | pb;
}

std::string sniffFormat(const std::vector<std::byte> &bytes)
{
    if (bytes.size() >= 8 && memcmp(bytes.data(), "\x89PNG\r\n\x1a\n", 8) == 0) return "png";
    if (bytes.size() >= 3 && (unsigned char)bytes[0] == 0xFF && (unsigned char)bytes[1] == 0xD8 &&
        (unsigned char)bytes[2] == 0xFF)
        return "jpeg";
    if (bytes.size() >= 6 && (memcmp(bytes.data(), "GIF87a", 6) == 0 || memcmp(bytes.data(), "GIF89a", 6) == 0))
        return "gif";
    if (bytes.size() >= 2 && bytes[0] == (std::byte)'B' && bytes[1] == (std::byte)'M') return "bmp";
    if (bytes.size() >= 2 && bytes[0] == (std::byte)'P' && (unsigned char)bytes[1] >= '1' && (unsigned char)bytes[1] <= '6')
        return "pnm";
    return "";
}

uint32_t crc32Buffer(const uint8_t *data, size_t len)
{
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

void pngChunk2(std::vector<uint8_t> &out, const char type[4], const uint8_t *data, size_t len)
{
    auto be32 = [&](uint32_t v) {
        out.push_back((uint8_t)(v >> 24)); out.push_back((uint8_t)(v >> 16));
        out.push_back((uint8_t)(v >> 8)); out.push_back((uint8_t)v);
    };
    std::vector<uint8_t> t;
    for (int i = 0; i < 4; i++) t.push_back((uint8_t)type[i]);
    std::vector<uint8_t> crcIn = t;
    if (len) crcIn.insert(crcIn.end(), data, data + len);
    be32((uint32_t)len);
    for (auto c : t) out.push_back(c);
    if (len) out.insert(out.end(), data, data + len);
    be32(crc32Buffer(crcIn.data(), crcIn.size()));
}

std::vector<std::byte> encodePng(ImageData *img)
{
    const int w = img->width, h = img->height;
    int channels = (img->colormodel == "argb") ? 4 : (img->colormodel == "grayscale" ? 1 : 3);
    uint8_t colorType = (img->colormodel == "argb") ? 6 : (img->colormodel == "grayscale" ? 0 : 2);

    // Raw scanlines with filter byte 0.
    std::vector<uint8_t> raw;
    raw.reserve((size_t)h * (1 + (size_t)w * channels));
    for (int y = 0; y < h; y++) {
        raw.push_back(0);
        for (int x = 0; x < w; x++) {
            uint8_t r, g, b, a;
            surfaceRGBA(img->surface, x, y, &r, &g, &b, &a);
            if (channels == 4) {
                raw.push_back(r); raw.push_back(g); raw.push_back(b); raw.push_back(a);
            } else if (channels == 1) {
                raw.push_back((uint8_t)((r * 77 + g * 150 + b * 29 + 128) / 256));
            } else {
                raw.push_back(r); raw.push_back(g); raw.push_back(b);
            }
        }
    }

    uLongf outLen = compressBound((uLong)raw.size());
    std::vector<uint8_t> z(outLen);
    int zr = compress2(z.data(), &outLen, raw.data(), (uLong)raw.size(), 6);
    if (zr != Z_OK) throw exception("PNG compression failed.");
    z.resize(outLen);

    std::vector<uint8_t> out;
    static const uint8_t sig[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    out.insert(out.end(), sig, sig + 8);
    uint8_t ihdr[13];
    ihdr[0] = (uint8_t)(w >> 24); ihdr[1] = (uint8_t)(w >> 16); ihdr[2] = (uint8_t)(w >> 8); ihdr[3] = (uint8_t)w;
    ihdr[4] = (uint8_t)(h >> 24); ihdr[5] = (uint8_t)(h >> 16); ihdr[6] = (uint8_t)(h >> 8); ihdr[7] = (uint8_t)h;
    ihdr[8] = 8;                // bit depth
    ihdr[9] = colorType;
    ihdr[10] = 0;               // compression
    ihdr[11] = 0;               // filter
    ihdr[12] = 0;               // interlace
    pngChunk2(out, "IHDR", ihdr, sizeof(ihdr));
    pngChunk2(out, "IDAT", z.data(), z.size());
    pngChunk2(out, "IEND", nullptr, 0);

    std::vector<std::byte> res;
    res.reserve(out.size());
    for (uint8_t c : out) res.push_back((std::byte)c);
    return res;
}

static cairo_status_t memReadCb(void *closure, unsigned char *data, unsigned int length)
{
    auto *r = (MemReader*)closure;
    if (length > r->len - r->pos) return CAIRO_STATUS_READ_ERROR;
    memcpy(data, r->data + r->pos, length);
    r->pos += length;
    return CAIRO_STATUS_SUCCESS;
}

ImageData *decodePng(const std::vector<std::byte> &bytes, const std::string &source)
{
    if (bytes.size() < 33) throw exception("ImageRead: Invalid PNG file.");
    const uint8_t *p = (const uint8_t*)bytes.data();
    if (memcmp(p, "\x89PNG\r\n\x1a\n", 8) != 0) throw exception("ImageRead: Invalid PNG file.");
    uint32_t len = ((uint32_t)p[8] << 24) | ((uint32_t)p[9] << 16) | ((uint32_t)p[10] << 8) | p[11];
    if (len != 13 || memcmp(p + 12, "IHDR", 4) != 0) throw exception("ImageRead: Invalid PNG file.");
    int w = ((int)p[16] << 24) | ((int)p[17] << 16) | ((int)p[18] << 8) | p[19];
    int h = ((int)p[20] << 24) | ((int)p[21] << 16) | ((int)p[22] << 8) | p[23];
    uint8_t colorType = p[25];
    if (w <= 0 || h <= 0) throw exception("ImageRead: Invalid PNG dimensions.");

    MemReader r = {p, bytes.size(), 0};
    cairo_surface_t *sf = cairo_image_surface_create_from_png_stream(memReadCb, &r);
    if (cairo_surface_status(sf) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(sf);
        throw exception("ImageRead: Unable to decode PNG image.");
    }
    auto *img = new ImageData;
    img->surface = sf;
    img->width = w;
    img->height = h;
    img->colormodel = (colorType == 0) ? "grayscale" : ((colorType == 4 || colorType == 6) ? "argb" : "rgb");
    img->colormodelType = "ComponentColorModel";
    img->source = source;
    img->sourceFormat = "png";
    img->sourceBytes = bytes;
    return img;
}

static void jpegErrorExit(j_common_ptr cinfo)
{
    auto *e = (JpegErrorMgr*)cinfo->err;
    longjmp(e->jb, 1);
}

static void jpegErrorOutput(j_common_ptr cinfo)
{
    (void)cinfo; // suppress diagnostics
}

std::vector<std::byte> encodeJpeg(ImageData *img, double quality)
{
    const int w = img->width, h = img->height;
    JpegErrorMgr err;
    jpeg_compress_struct cinfo;
    cinfo.err = jpeg_std_error(&err.pub);
    err.pub.error_exit = jpegErrorExit;
    err.pub.output_message = jpegErrorOutput;

    std::vector<std::byte> out;
    unsigned char *mem = nullptr;
    unsigned long memLen = 0;

    if (setjmp(err.jb)) {
        jpeg_destroy_compress(&cinfo);
        if (mem) free(mem);
        throw exception("ImageWrite: Unable to encode JPEG image.");
    }

    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &mem, &memLen);

    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, (int)(quality * 100 + 0.5), TRUE);

    // Match ImageIO's JFIF 1.02 header so ImageGetMetadata values agree.
    cinfo.write_JFIF_header = FALSE;
    cinfo.density_unit = 0;
    cinfo.X_density = 1;
    cinfo.Y_density = 1;

    jpeg_start_compress(&cinfo, TRUE);
    static const unsigned char jfif[14] = {'J', 'F', 'I', 'F', 0, 1, 2, 0, 0, 1, 0, 1, 0, 0};
    jpeg_write_marker(&cinfo, JPEG_APP0, (const JOCTET*)jfif, sizeof(jfif));

    std::vector<uint8_t> row((size_t)w * 3);
    while (cinfo.next_scanline < cinfo.image_height) {
        uint8_t *rp = row.data();
        for (int x = 0; x < w; x++) {
            uint8_t r, g, b, a;
            surfaceRGBA(img->surface, x, (int)cinfo.next_scanline, &r, &g, &b, &a);
            rp[x * 3] = r; rp[x * 3 + 1] = g; rp[x * 3 + 2] = b;
        }
        jpeg_write_scanlines(&cinfo, (JSAMPARRAY)&rp, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    out.reserve(memLen);
    for (unsigned long i = 0; i < memLen; i++) out.push_back((std::byte)mem[i]);
    free(mem);
    return out;
}

ImageData *decodeJpeg(const std::vector<std::byte> &bytes, const std::string &source)
{
    JpegErrorMgr err;
    jpeg_decompress_struct cinfo;
    cinfo.err = jpeg_std_error(&err.pub);
    err.pub.error_exit = jpegErrorExit;
    err.pub.output_message = jpegErrorOutput;

    if (setjmp(err.jb)) {
        jpeg_destroy_decompress(&cinfo);
        throw exception("ImageRead: Unable to decode JPEG image.");
    }
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, (unsigned char*)bytes.data(), (unsigned long)bytes.size());
    jpeg_read_header(&cinfo, TRUE);
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    int w = cinfo.output_width, h = cinfo.output_height;
    if (w <= 0 || h <= 0) {
        jpeg_destroy_decompress(&cinfo);
        throw exception("ImageRead: Invalid JPEG dimensions.");
    }
    auto *img = imageAlloc(w, h);
    img->colormodel = "rgb";
    img->colormodelType = "ComponentColorModel";
    img->source = source;
    img->sourceFormat = "jpeg";
    img->sourceBytes = bytes;

    std::vector<uint8_t> row((size_t)w * 3);
    while (cinfo.output_scanline < cinfo.output_height) {
        int y = (int)cinfo.output_scanline;
        uint8_t *rp = row.data();
        jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&rp, 1);
        for (int x = 0; x < w; x++) {
            surfaceSetRGBA(img->surface, x, y, rp[x * 3], rp[x * 3 + 1], rp[x * 3 + 2], 255);
        }
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return img;
}

std::vector<uint8_t> gifLzwDecode(const std::vector<uint8_t> &data, int minCodeSize)
{
    std::vector<uint8_t> out;
    const int clearCode = 1 << minCodeSize;
    const int eoiCode = clearCode + 1;
    int codeSize = minCodeSize + 1;
    int nextCode = eoiCode + 1;
    std::vector<std::vector<uint8_t>> dict;
    dict.resize(4096);

    auto reset = [&]() {
        dict.assign(4096, {});
        for (int i = 0; i < clearCode; i++) dict[i] = {(uint8_t)i};
        nextCode = eoiCode + 1;
        codeSize = minCodeSize + 1;
    };
    reset();

    uint32_t bitbuf = 0;
    int bitcnt = 0;
    size_t pos = 0;
    auto getCode = [&]() -> int {
        while (bitcnt < codeSize) {
            if (pos >= data.size()) return -1;
            bitbuf |= ((uint32_t)data[pos++]) << bitcnt;
            bitcnt += 8;
        }
        int c = (int)(bitbuf & ((1u << codeSize) - 1));
        bitbuf >>= codeSize;
        bitcnt -= codeSize;
        return c;
    };

    std::vector<uint8_t> prev;
    bool first = true;
    while (true) {
        int code = getCode();
        if (code < 0) break;
        if (code == clearCode) { reset(); first = true; continue; }
        if (code == eoiCode) break;
        std::vector<uint8_t> entry;
        if (code < nextCode) {
            entry = dict[code];
        } else if (code == nextCode && !first && !prev.empty()) {
            entry = prev;
            entry.push_back(prev[0]);
        } else {
            break;
        }
        for (uint8_t c : entry) out.push_back(c);
        if (!first && nextCode < 4096) {
            dict[nextCode] = prev;
            dict[nextCode].push_back(entry[0]);
            nextCode++;
            if (nextCode == (1 << codeSize) && codeSize < 12) codeSize++;
        }
        prev = entry;
        first = false;
    }
    return out;
}

void gifBuildPalette(ImageData *img, std::vector<uint8_t> &palette, std::vector<uint8_t> &indices)
{
    const int w = img->width, h = img->height;
    std::map<uint32_t, int> seen; // rgb -> index
    std::vector<uint32_t> colors;
    std::vector<int> perPixel((size_t)w * h, 0);
    bool quantize = false;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t r, g, b, a;
            surfaceRGBA(img->surface, x, y, &r, &g, &b, &a);
            uint32_t key = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            auto it = seen.find(key);
            if (it != seen.end()) {
                perPixel[y * w + x] = it->second;
            } else {
                int idx = (int)colors.size();
                if (idx >= 256) { quantize = true; }
                if (idx < 256) {
                    seen.emplace(key, idx);
                    colors.push_back(key);
                    perPixel[y * w + x] = idx;
                } else {
                    perPixel[y * w + x] = -1;
                }
            }
        }
    }

    if (quantize) {
        // Fall back to a fixed 3-3-2 palette.
        colors.clear();
        seen.clear();
        for (int r = 0; r < 8; r++)
            for (int g = 0; g < 8; g++)
                for (int b = 0; b < 4; b++)
                    colors.push_back(((uint32_t)(r * 36) << 16) | ((uint32_t)(g * 36) << 8) | (uint32_t)(b * 85));
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                uint8_t r, g, b, a;
                surfaceRGBA(img->surface, x, y, &r, &g, &b, &a);
                perPixel[y * w + x] = ((r >> 5) << 3) | ((g >> 5) << 1) | (b >> 6);
            }
        }
    }

    palette.clear();
    for (uint32_t c : colors) {
        palette.push_back((uint8_t)(c >> 16));
        palette.push_back((uint8_t)(c >> 8));
        palette.push_back((uint8_t)c);
    }
    // pad to a power of two
    int size = 1;
    while (size < (int)colors.size()) size <<= 1;
    while ((int)palette.size() < size * 3) palette.push_back(0);

    indices.clear();
    indices.reserve((size_t)w * h);
    for (int v : perPixel) indices.push_back((uint8_t)v);
}

void gifLzwEncode(const std::vector<uint8_t> &pixels, int minCodeSize, std::vector<uint8_t> &out)
{
    const int clearCode = 1 << minCodeSize;
    const int eoiCode = clearCode + 1;
    int codeSize = minCodeSize + 1;
    int nextCode = eoiCode + 1;
    std::map<std::vector<uint8_t>, int> dict;
    for (int i = 0; i < clearCode; i++) dict.emplace(std::vector<uint8_t>{(uint8_t)i}, i);

    uint32_t bitbuf = 0;
    int bitcnt = 0;
    auto emit = [&](int code) {
        bitbuf |= ((uint32_t)code) << bitcnt;
        bitcnt += codeSize;
        while (bitcnt >= 8) {
            out.push_back((uint8_t)(bitbuf & 0xFF));
            bitbuf >>= 8;
            bitcnt -= 8;
        }
    };
    auto reset = [&]() {
        emit(clearCode);
        dict.clear();
        for (int i = 0; i < clearCode; i++) dict.emplace(std::vector<uint8_t>{(uint8_t)i}, i);
        nextCode = eoiCode + 1;
        codeSize = minCodeSize + 1;
    };

    reset();
    std::vector<uint8_t> w;
    for (size_t i = 0; i < pixels.size(); i++) {
        std::vector<uint8_t> wc = w;
        wc.push_back(pixels[i]);
        auto it = dict.find(wc);
        if (it != dict.end()) {
            w = wc;
        } else {
            emit(dict.at(w));
            if (nextCode < 4096) {
                dict[wc] = nextCode;
                nextCode++;
                if (nextCode == (1 << codeSize) && codeSize < 12) codeSize++;
            }
            if (nextCode >= 4096) {
                reset();
                w = {pixels[i]};
                continue;
            }
            w = {pixels[i]};
        }
    }
    if (!w.empty()) emit(dict.at(w));
    emit(eoiCode);
    if (bitcnt > 0) out.push_back((uint8_t)(bitbuf & 0xFF));
}

std::vector<std::byte> encodeGif(ImageData *img)
{
    const int w = img->width, h = img->height;
    std::vector<uint8_t> palette, indices;
    gifBuildPalette(img, palette, indices);

    int paletteBits = 2;
    while ((1 << paletteBits) < (int)(palette.size() / 3)) paletteBits++;
    if (paletteBits < 2) paletteBits = 2;
    if (paletteBits > 8) paletteBits = 8;

    std::vector<uint8_t> codes;
    gifLzwEncode(indices, paletteBits, codes);

    std::vector<uint8_t> body;
    body.insert(body.end(), {'G', 'I', 'F', '8', '9', 'a'});
    auto le16 = [&](int v) { body.push_back((uint8_t)(v & 0xFF)); body.push_back((uint8_t)((v >> 8) & 0xFF)); };
    le16(w); le16(h);
    body.push_back((uint8_t)(0x80u | ((paletteBits - 1) & 0x07u))); // GCT flag + size
    body.push_back(0); // background color index
    body.push_back(0); // aspect ratio
    body.insert(body.end(), palette.begin(), palette.end());
    // Image descriptor (no local color table, no interlace).
    body.push_back(0x2C);
    le16(0); le16(0); le16(w); le16(h);
    body.push_back(0);
    // LZW min code size + sub-blocks.
    body.push_back((uint8_t)paletteBits);
    for (size_t i = 0; i < codes.size(); i += 255) {
        size_t n = std::min<size_t>(255, codes.size() - i);
        body.push_back((uint8_t)n);
        body.insert(body.end(), codes.begin() + i, codes.begin() + i + n);
    }
    body.push_back(0);
    body.push_back(0x3B); // trailer

    std::vector<std::byte> res;
    res.reserve(body.size());
    for (uint8_t c : body) res.push_back((std::byte)c);
    return res;
}

ImageData *decodeGif(const std::vector<std::byte> &bytes, const std::string &source)
{
    if (bytes.size() < 13) throw exception("ImageRead: Invalid GIF file.");
    const uint8_t *d = (const uint8_t*)bytes.data();
    size_t n = bytes.size();
    if (memcmp(d, "GIF87a", 6) != 0 && memcmp(d, "GIF89a", 6) != 0) throw exception("ImageRead: Invalid GIF file.");
    int w = d[6] | (d[7] << 8);
    int h = d[8] | (d[9] << 8);
    uint8_t gctFlags = d[10];
    int gctSize = gctFlags & 0x07;
    int gctLen = (gctFlags & 0x80) ? 3 * (1 << (gctSize + 1)) : 0;
    if (w <= 0 || h <= 0) throw exception("ImageRead: Invalid GIF dimensions.");
    size_t pos = 13 + (size_t)gctLen;
    if (pos > n) throw exception("ImageRead: Invalid GIF file.");

    auto *img = imageAlloc(w, h);
    img->colormodel = "rgb";
    img->colormodelType = "ComponentColorModel";
    img->source = source;
    img->sourceFormat = "gif";
    img->sourceBytes = bytes;

    std::vector<uint8_t> outPixels((size_t)w * h, 0);
    std::vector<uint8_t> activePal;

    while (pos < n) {
        uint8_t block = d[pos++];
        if (block == 0x3B) break;                       // trailer
        if (block == 0x21) {                            // extension
            if (pos >= n) break;
            pos++;                                      // label
            while (pos < n) {
                uint8_t sz = d[pos++];
                if (sz == 0) break;
                pos += sz;
            }
            continue;
        }
        if (block != 0x2C) break;                       // image descriptor
        if (pos + 9 > n) break;
        int il = d[pos] | (d[pos + 1] << 8);
        int it = d[pos + 2] | (d[pos + 3] << 8);
        int iw = d[pos + 4] | (d[pos + 5] << 8);
        int ih = d[pos + 6] | (d[pos + 7] << 8);
        uint8_t imgFlags = d[pos + 8];
        pos += 9;
        int interlace = (imgFlags & 0x40) != 0;
        int lctLen = (imgFlags & 0x80) ? 3 * (1 << ((imgFlags & 0x07) + 1)) : 0;
        if (lctLen) {
            if (pos + lctLen > n) break;
            activePal.assign(d + pos, d + pos + lctLen);
            pos += lctLen;
        } else {
            if (gctLen == 0) break;
            activePal.assign(d + 13, d + 13 + gctLen);
        }
        if (pos >= n) break;
        int minCode = d[pos++];
        std::vector<uint8_t> stream;
        while (pos < n) {
            uint8_t sz = d[pos++];
            if (sz == 0) break;
            if (pos + sz > n) break;
            stream.insert(stream.end(), d + pos, d + pos + sz);
            pos += sz;
        }
        std::vector<uint8_t> pix = gifLzwDecode(stream, minCode);

        // Map the (possibly interlaced) storage rows to display rows: passes
        // start at 0/4/2/1 with steps 8/8/4/2.
        auto interlaceRow = [&](int y) -> int {
            int count1 = (ih + 7) / 8;
            if (y < count1) return y * 8;
            int count2 = (ih > 4) ? (ih - 4 + 7) / 8 : 0;
            y -= count1;
            if (y < count2) return 4 + y * 8;
            int count3 = (ih > 2) ? (ih - 2 + 3) / 4 : 0;
            y -= count2;
            if (y < count3) return 2 + y * 4;
            y -= count3;
            return 1 + y * 2;
        };

        for (int y = 0; y < ih; y++) {
            int ty = interlace ? interlaceRow(y) : y;
            int dy = it + ty;
            if (dy < 0 || dy >= h) continue;
            for (int x = 0; x < iw; x++) {
                int dx = il + x;
                if (dx < 0 || dx >= w) continue;
                size_t src = (size_t)y * iw + x;
                if (src >= pix.size()) break;
                uint8_t ci = pix[src];
                size_t idx = ((size_t)dy) * w + dx;
                if (idx < outPixels.size() && (size_t)ci * 3 + 2 < activePal.size()) {
                    outPixels[idx] = ci;
                }
            }
        }
        break; // ColdFusion reads the first frame of an animated GIF
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t ci = outPixels[y * w + x];
            if ((size_t)ci * 3 + 2 >= activePal.size()) continue;
            surfaceSetRGBA(img->surface, x, y, activePal[ci * 3], activePal[ci * 3 + 1], activePal[ci * 3 + 2], 255);
        }
    }
    return img;
}

std::vector<std::byte> encodeBmp(ImageData *img)
{
    const int w = img->width, h = img->height;
    int bpp = (img->colormodel == "argb") ? 32 : 24;
    int rowBytes = ((w * bpp + 31) / 32) * 4;
    int pixelBytes = rowBytes * h;
    uint32_t fileSize = 54u + (uint32_t)pixelBytes;

    std::vector<uint8_t> out;
    out.reserve(fileSize);
    auto le32 = [&](uint32_t v) {
        out.push_back((uint8_t)(v & 0xFF)); out.push_back((uint8_t)((v >> 8) & 0xFF));
        out.push_back((uint8_t)((v >> 16) & 0xFF)); out.push_back((uint8_t)((v >> 24) & 0xFF));
    };
    auto le16 = [&](int v) { out.push_back((uint8_t)(v & 0xFF)); out.push_back((uint8_t)((v >> 8) & 0xFF)); };

    out.insert(out.end(), {'B', 'M'});
    le32(fileSize);
    le32(0);
    le32(54);
    le32(40);                // BITMAPINFOHEADER
    le32((uint32_t)w);
    le32((uint32_t)h);
    le16(1);
    le16(bpp);
    le32(0);                 // BI_RGB
    le32((uint32_t)pixelBytes);
    le32(2835); le32(2835);  // 72 dpi
    le32(0); le32(0);

    for (int y = h - 1; y >= 0; y--) {
        std::vector<uint8_t> row((size_t)rowBytes, 0);
        for (int x = 0; x < w; x++) {
            uint8_t r, g, b, a;
            surfaceRGBA(img->surface, x, y, &r, &g, &b, &a);
            size_t o = (size_t)x * (bpp / 8);
            row[o] = b; row[o + 1] = g; row[o + 2] = r;
            if (bpp == 32) row[o + 3] = 0;
        }
        out.insert(out.end(), row.begin(), row.end());
    }

    std::vector<std::byte> res;
    res.reserve(out.size());
    for (uint8_t c : out) res.push_back((std::byte)c);
    return res;
}

ImageData *decodeBmp(const std::vector<std::byte> &bytes, const std::string &source)
{
    if (bytes.size() < 54) throw exception("ImageRead: Invalid BMP file.");
    const uint8_t *d = (const uint8_t*)bytes.data();
    if (d[0] != 'B' || d[1] != 'M') throw exception("ImageRead: Invalid BMP file.");
    auto rd32 = [&](size_t o) { return (uint32_t)d[o] | ((uint32_t)d[o + 1] << 8) | ((uint32_t)d[o + 2] << 16) | ((uint32_t)d[o + 3] << 24); };
    auto rd16 = [&](size_t o) { return (uint16_t)(d[o] | (d[o + 1] << 8)); };
    uint32_t dataOff = rd32(10);
    uint32_t headerSize = rd32(14);
    int32_t width = (int32_t)rd32(18);
    int32_t height = (int32_t)rd32(22);
    uint16_t planes = rd16(26);
    uint16_t bits = rd16(28);
    uint32_t compression = rd32(30);
    if (planes != 1) throw exception("ImageRead: Unsupported BMP file.");
    bool topDown = height < 0;
    int h = topDown ? -height : height;
    if (width <= 0 || h <= 0) throw exception("ImageRead: Invalid BMP dimensions.");

    size_t palOff = 14 + headerSize;
    size_t palCount = 0;
    if (bits <= 8) {
        uint32_t colorsUsed = rd32(46);
        palCount = colorsUsed ? colorsUsed : (1u << bits);
        if (palOff + palCount * 4 > bytes.size()) throw exception("ImageRead: Invalid BMP palette.");
    }

    auto *img = imageAlloc(width, h);
    img->colormodel = "rgb";
    img->colormodelType = "ComponentColorModel";
    img->source = source;
    img->sourceFormat = "bmp";
    img->sourceBytes = bytes;

    int rowBytes = ((int)width * (int)bits + 31) / 32 * 4;
    if (dataOff > bytes.size()) throw exception("ImageRead: Invalid BMP file.");
    if (compression == 0) {
        // BI_RGB
        for (int r = 0; r < h; r++) {
            int y = topDown ? r : (h - 1 - r);
            size_t base = dataOff + (size_t)r * rowBytes;
            if (base + rowBytes > bytes.size()) break;
            for (int x = 0; x < width; x++) {
                uint8_t r8, g8, b8;
                if (bits == 24) {
                    size_t o = base + (size_t)x * 3;
                    b8 = d[o]; g8 = d[o + 1]; r8 = d[o + 2];
                } else if (bits == 32) {
                    size_t o = base + (size_t)x * 4;
                    b8 = d[o]; g8 = d[o + 1]; r8 = d[o + 2];
                } else if (bits == 8) {
                    uint8_t idx = d[base + x];
                    size_t po = palOff + (size_t)idx * 4;
                    if (po + 3 > bytes.size()) continue;
                    b8 = d[po]; g8 = d[po + 1]; r8 = d[po + 2];
                } else if (bits == 4) {
                    uint8_t v = d[base + x / 2];
                    uint8_t idx = (x & 1) ? (v & 0x0F) : (v >> 4);
                    size_t po = palOff + (size_t)idx * 4;
                    if (po + 3 > bytes.size()) continue;
                    b8 = d[po]; g8 = d[po + 1]; r8 = d[po + 2];
                } else if (bits == 1) {
                    uint8_t v = d[base + x / 8];
                    uint8_t idx = (v >> (7 - (x & 7))) & 1;
                    size_t po = palOff + (size_t)idx * 4;
                    if (po + 3 > bytes.size()) continue;
                    b8 = d[po]; g8 = d[po + 1]; r8 = d[po + 2];
                } else {
                    continue;
                }
                surfaceSetRGBA(img->surface, x, y, r8, g8, b8, 255);
            }
        }
    } else if (compression == 1 && bits == 8) {
        // BI_RLE8
        size_t p = dataOff;
        int x = 0, y = topDown ? 0 : h - 1;
        while (p + 1 < bytes.size()) {
            uint8_t c1 = d[p++], c2 = d[p++];
            if (c1) {
                // literal run of c1 pixels
                for (int i = 0; i < c1; i++) {
                    uint8_t idx = c2;
                    if (i == 0) idx = c2;
                    if (x < width) {
                        size_t po = palOff + (size_t)idx * 4;
                        if (po + 3 <= bytes.size())
                            surfaceSetRGBA(img->surface, x, y, d[po + 2], d[po + 1], d[po], 255);
                    }
                    x++;
                }
            } else {
                if (c2 == 0) { // end of line
                    x = 0;
                    y += topDown ? 1 : -1;
                } else if (c2 == 1) { // end of bitmap
                    break;
                } else if (c2 == 2) { // delta
                    if (p + 1 < bytes.size()) {
                        x += d[p++];
                        int dy = d[p++];
                        y += topDown ? dy : -dy;
                    }
                } else { // literal run
                    int count = c2;
                    if (p + count > bytes.size()) break;
                    for (int i = 0; i < count; i++) {
                        uint8_t idx = d[p + i];
                        if (x < width) {
                            size_t po = palOff + (size_t)idx * 4;
                            if (po + 3 <= bytes.size())
                                surfaceSetRGBA(img->surface, x, y, d[po + 2], d[po + 1], d[po], 255);
                        }
                        x++;
                    }
                    p += count;
                    if (count & 1) p++; // word-aligned
                }
            }
        }
    } else if (compression == 2 && bits == 4) {
        // BI_RLE4
        size_t p = dataOff;
        int x = 0, y = topDown ? 0 : h - 1;
        bool nibble = false;
        uint8_t lastNib = 0;
        while (p < bytes.size()) {
            if (p + 1 >= bytes.size()) break;
            uint8_t c1 = d[p++], c2 = d[p++];
            if (c1) {
                int nibbles = c1;
                uint8_t cur = c2;
                for (int i = 0; i < nibbles; i++) {
                    uint8_t idx = (i & 1) ? (cur & 0x0F) : (cur >> 4);
                    if (i & 1) {
                        if (p < bytes.size() && i == nibbles - 1) { }
                    }
                    if (i & 1) cur = d[p];
                    if (x < width) {
                        size_t po = palOff + (size_t)idx * 4;
                        if (po + 3 <= bytes.size())
                            surfaceSetRGBA(img->surface, x, y, d[po + 2], d[po + 1], d[po], 255);
                    }
                    x++;
                }
                if (c1 & 1) { /* odd nibble count, next byte is pad? */ }
            } else {
                if (c2 == 0) {
                    x = 0;
                    y += topDown ? 1 : -1;
                } else if (c2 == 1) {
                    break;
                } else if (c2 == 2) {
                    if (p + 1 < bytes.size()) {
                        x += d[p++];
                        int dy = d[p++];
                        y += topDown ? dy : -dy;
                    }
                } else {
                    int count = c2;
                    if (p + (count + 1) / 2 > bytes.size()) break;
                    for (int i = 0; i < count; i++) {
                        uint8_t idx;
                        if (i & 1) idx = d[p + i / 2] & 0x0F;
                        else idx = d[p + i / 2] >> 4;
                        if (x < width) {
                            size_t po = palOff + (size_t)idx * 4;
                            if (po + 3 <= bytes.size())
                                surfaceSetRGBA(img->surface, x, y, d[po + 2], d[po + 1], d[po], 255);
                        }
                        x++;
                    }
                    p += (count + 1) / 2;
                    if (((count + 1) / 2) & 1) p++;
                }
            }
        }
    } else {
        throw exception("ImageRead: Unsupported BMP compression.");
    }
    return img;
}

std::vector<std::byte> encodePnm(ImageData *img)
{
    const int w = img->width, h = img->height;
    bool gray = (img->colormodel == "grayscale");
    std::vector<uint8_t> out;
    out.reserve((size_t)h * ((size_t)w * (gray ? 1 : 3)) + 32);
    std::string header = gray ? "P5\n" : "P6\n";
    header += std::to_string(w) + " " + std::to_string(h) + "\n255\n";
    for (char c : header) out.push_back((uint8_t)c);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t r, g, b, a;
            surfaceRGBA(img->surface, x, y, &r, &g, &b, &a);
            if (gray) out.push_back((uint8_t)((r * 77 + g * 150 + b * 29 + 128) / 256));
            else { out.push_back(r); out.push_back(g); out.push_back(b); }
        }
    }
    std::vector<std::byte> res;
    res.reserve(out.size());
    for (uint8_t c : out) res.push_back((std::byte)c);
    return res;
}

ImageData *decodePnm(const std::vector<std::byte> &bytes, const std::string &source)
{
    if (bytes.size() < 4) throw exception("ImageRead: Invalid PNM file.");
    const uint8_t *d = (const uint8_t*)bytes.data();
    size_t n = bytes.size();
    if (d[0] != 'P') throw exception("ImageRead: Invalid PNM file.");
    char magic = (char)d[1];
    if (magic < '1' || magic > '6') throw exception("ImageRead: Invalid PNM file.");

    size_t pos = 2;
    auto nextToken = [&]() -> std::string {
        // skip whitespace and comments
        while (pos < n) {
            if (d[pos] == '#') { while (pos < n && d[pos] != '\n') pos++; continue; }
            if (isspace(d[pos])) { pos++; continue; }
            break;
        }
        size_t start = pos;
        while (pos < n && !isspace(d[pos]) && d[pos] != '#') pos++;
        return std::string((const char*)d + start, pos - start);
    };

    std::string wStr = nextToken();
    std::string hStr = nextToken();
    if (wStr.empty() || hStr.empty()) throw exception("ImageRead: Invalid PNM file.");
    int w = atoi(wStr.c_str());
    int h = atoi(hStr.c_str());
    if (w <= 0 || h <= 0) throw exception("ImageRead: Invalid PNM dimensions.");

    bool binary = (magic >= '4');
    int maxVal = 255;
    if (magic == '5' || magic == '6') {
        std::string m = nextToken();
        maxVal = atoi(m.c_str());
        if (maxVal <= 0) maxVal = 255;
    }

    // For binary formats the tokenizer left us right before the pixel data
    // (single whitespace char remains).
    auto *img = imageAlloc(w, h);
    img->colormodel = (magic == '6' || magic == '3') ? "rgb" : "grayscale";
    img->colormodelType = "ComponentColorModel";
    img->source = source;
    img->sourceFormat = "pnm";
    img->sourceBytes = bytes;

    auto setPixel = [&](int x, int y, int r, int g, int b) {
        if (x >= 0 && x < w && y >= 0 && y < h)
            surfaceSetRGBA(img->surface, x, y, (uint8_t)r, (uint8_t)g, (uint8_t)b, 255);
    };
    int scale = (maxVal > 0 && maxVal != 255) ? (255 + maxVal / 2) / maxVal : 1;

    if (magic == '1' || magic == '2' || magic == '3') {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (magic == '1') {
                    std::string t = nextToken();
                    setPixel(x, y, t == "1" ? 0 : 255, t == "1" ? 0 : 255, t == "1" ? 0 : 255);
                } else if (magic == '2') {
                    int v = atoi(nextToken().c_str()) * scale;
                    setPixel(x, y, v, v, v);
                } else {
                    int r = atoi(nextToken().c_str()) * scale;
                    int g = atoi(nextToken().c_str()) * scale;
                    int b = atoi(nextToken().c_str()) * scale;
                    setPixel(x, y, r, g, b);
                }
            }
        }
    } else {
        size_t dataStart = pos;
        // Skip the whitespace separating the header token from the raster.
        while (dataStart < n && isspace(d[dataStart])) dataStart++;
        // 'P4' pixel rows are packed bits; compute row size.
        int rowBytes = magic == '4' ? ((w + 7) / 8) : ((magic == '5' ? w : w * 3) * (maxVal > 255 ? 2 : 1));
        for (int y = 0; y < h; y++) {
            size_t base = dataStart + (size_t)y * rowBytes;
            if (base + rowBytes > n) break;
            for (int x = 0; x < w; x++) {
                if (magic == '4') {
                    uint8_t byte = d[base + x / 8];
                    int v = (byte >> (7 - (x & 7))) & 1;
                    setPixel(x, y, v ? 0 : 255, v ? 0 : 255, v ? 0 : 255);
                } else if (magic == '5') {
                    int v = d[base + x] * scale;
                    setPixel(x, y, v, v, v);
                } else {
                    int r = d[base + x * 3] * scale;
                    int g = d[base + x * 3 + 1] * scale;
                    int b = d[base + x * 3 + 2] * scale;
                    setPixel(x, y, r, g, b);
                }
            }
        }
    }
    return img;
}

ImageData *imageFromBytes(const std::vector<std::byte> &bytes, const std::string &formatHint, const std::string &source)
{
    std::string fmt = formatHint;
    if (fmt.empty()) fmt = sniffFormat(bytes);
    if (fmt.empty()) throw exception("The file is not a valid image file.");
    if (fmt == "png") return decodePng(bytes, source);
    if (fmt == "jpeg") return decodeJpeg(bytes, source);
    if (fmt == "gif") return decodeGif(bytes, source);
    if (fmt == "bmp") return decodeBmp(bytes, source);
    if (fmt == "pnm") return decodePnm(bytes, source);
    throw exception(string(("Unsupported image format: " + fmt).c_str()));
}

std::vector<std::byte> encodeImage(ImageData *img, const std::string &format, double quality)
{
    std::string f = toLower(format);
    if (f == "jpeg" || f == "jpg" || f == "jfif") return encodeJpeg(img, quality);
    if (f == "png") return encodePng(img);
    if (f == "gif") return encodeGif(img);
    if (f == "bmp") return encodeBmp(img);
    if (f == "pnm" || f == "ppm" || f == "pgm" || f == "pbm") return encodePnm(img);
    throw exception(string(("Unable to write image. Unsupported format: " + format).c_str()));
}

bool cfColorName(const std::string &lower, uint32_t &out)
{
    static const std::map<std::string, uint32_t> colors = {
        {"black", 0x000000u}, {"white", 0xFFFFFFu}, {"red", 0xFF0000u},
        {"blue", 0x0000FFu}, {"green", 0x00FF00u}, {"gray", 0x808080u},
        {"darkgray", 0x404040u}, {"pink", 0xFFAFAFu}, {"orange", 0xFFC800u},
        {"magenta", 0xFF00FFu}, {"yellow", 0xFFFF00u}, {"cyan", 0x00FFFFu},
    };
    auto it = colors.find(lower);
    if (it == colors.end()) return false;
    out = it->second;
    return true;
}

void colorError(const std::string &message)
{
    imageThrow("Application", message, "Verify your inputs. " + message);
}

bool parseIntStrict(const std::string &s, int &out)
{
    if (s.empty()) return false;
    size_t i = 0;
    bool neg = false;
    if (s[0] == '+' || s[0] == '-') {
        neg = s[0] == '-';
        i = 1;
    }
    if (i >= s.size()) return false;
    long long v = 0;
    for (; i < s.size(); i++) {
        char c = s[i];
        if (c < '0' || c > '9') return false;
        v = v * 10 + (c - '0');
    }
    out = neg ? (int)-v : (int)v;
    return true;
}

uint32_t parseDrawColor(const std::string &raw)
{
    if (raw.empty()) {
        throw exception(string("java.lang.StringIndexOutOfBoundsException"),
                        string("Index 0 out of bounds for length 0"), string());
    }

    if (raw.find(',') != std::string::npos) {
        std::vector<std::string> parts;
        size_t start = 0;
        for (size_t i = 0; i <= raw.size(); i++) {
            if (i == raw.size() || raw[i] == ',') {
                parts.push_back(raw.substr(start, i - start));
                start = i + 1;
            }
        }
        if (parts.size() == 2) {
            std::string len = std::to_string(parts[1].size());
            throw exception(string("java.lang.StringIndexOutOfBoundsException"),
                            string(("Range [0, -1) out of bounds for length " + len).c_str()), string());
        }
        if (parts.size() != 3) colorError(kColorRGB);
        int r, g, b;
        if (!parseIntStrict(parts[0], r) || !parseIntStrict(parts[1], g) || !parseIntStrict(parts[2], b))
            colorError(kColorRGB);
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) colorError(kColorRGB);
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    if (raw[0] == '#') {
        std::string h = raw.substr(1);
        if (h.size() != 6) colorError(kColorHex);
        for (char c : h) {
            if (!isxdigit((unsigned char)c)) colorError(kColorHex);
        }
        return (uint32_t)strtoul(h.c_str(), nullptr, 16);
    }

    if (raw.size() == 6) {
        bool hex = true;
        for (char c : raw) {
            if (!isxdigit((unsigned char)c)) {
                hex = false;
                break;
            }
        }
        if (hex) return (uint32_t)strtoul(raw.c_str(), nullptr, 16);
    }

    uint32_t named;
    if (cfColorName(toLower(raw), named)) return named;
    colorError(kColorRequired);
    return 0;
}

void setupSource(cairo_t *cr, ImageData *img)
{
    uint32_t dc = img->drawingColor;
    double a = 1.0;
    if (img->transparency > 0) {
        a = 1.0 - img->transparency / 100.0;
        if (a < 0) a = 0;
    }
    cairo_set_source_rgba(cr, ((dc >> 16) & 0xFF) / 255.0, ((dc >> 8) & 0xFF) / 255.0,
                          (dc & 0xFF) / 255.0, a);
    if (img->antialias) cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    else cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
}

void setupStroke(cairo_t *cr, ImageData *img)
{
    cairo_set_line_width(cr, img->strokeWidth);
    cairo_set_line_cap(cr, img->strokeCaps == "round" ? CAIRO_LINE_CAP_ROUND
                         : img->strokeCaps == "square" ? CAIRO_LINE_CAP_SQUARE : CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, img->strokeJoins == "round" ? CAIRO_LINE_JOIN_ROUND
                          : img->strokeJoins == "bevel" ? CAIRO_LINE_JOIN_BEVEL : CAIRO_LINE_JOIN_MITER);
    cairo_set_miter_limit(cr, img->strokeMiterLimit);
    if (!img->strokeDash.empty())
        cairo_set_dash(cr, img->strokeDash.data(), (int)img->strokeDash.size(), img->strokeDashPhase);
}

void paintShape(ImageData *img, const std::function<void(cairo_t*)> &draw)
{
    if (!img->xorMode) {
        cairo_t *cr = cairo_create(img->surface);
        setupSource(cr, img);
        setupStroke(cr, img);
        if (img->hasDrawingTransform) cairo_set_matrix(cr, &img->drawingTransform);
        draw(cr);
        cairo_destroy(cr);
        return;
    }

    cairo_surface_t *mask = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, img->width, img->height);
    cairo_t *mcr = cairo_create(mask);
    cairo_set_source_rgba(mcr, 1, 1, 1, 1);
    if (img->antialias) cairo_set_antialias(mcr, CAIRO_ANTIALIAS_DEFAULT);
    else cairo_set_antialias(mcr, CAIRO_ANTIALIAS_NONE);
    setupStroke(mcr, img);
    if (img->hasDrawingTransform) cairo_set_matrix(mcr, &img->drawingTransform);
    draw(mcr);
    cairo_destroy(mcr);

    const uint32_t xc = img->xorColor;
    const uint32_t dc = img->drawingColor;
    int w = img->width, h = img->height;
    const uint32_t *md = (const uint32_t*)cairo_image_surface_get_data(mask);
    int mstride = cairo_image_surface_get_stride(mask);
    uint8_t r, g, b, a;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint32_t ma = (md[y * mstride / 4 + x] >> 24) & 0xFF;
            if (!ma) continue;
            surfaceRGBA(img->surface, x, y, &r, &g, &b, &a);
            uint8_t nr = (uint8_t)((dc >> 16) & 0xFF) ^ (uint8_t)((xc >> 16) & 0xFF) ^ r;
            uint8_t ng = (uint8_t)((dc >> 8) & 0xFF) ^ (uint8_t)((xc >> 8) & 0xFF) ^ g;
            uint8_t nb = (uint8_t)(dc & 0xFF) ^ (uint8_t)(xc & 0xFF) ^ b;
            if (ma == 255) {
                surfaceSetRGBA(img->surface, x, y, nr, ng, nb, 255);
            } else {
                double cov = ma / 255.0;
                uint8_t fr = (uint8_t)(r + (nr - r) * cov);
                uint8_t fg = (uint8_t)(g + (ng - g) * cov);
                uint8_t fb = (uint8_t)(b + (nb - b) * cov);
                surfaceSetRGBA(img->surface, x, y, fr, fg, fb, 255);
            }
        }
    }
    cairo_surface_destroy(mask);
}

void fillRectColor(ImageData *img, int x, int y, int w, int h, uint32_t rgb)
{
    if (!img->xorMode) {
        cairo_t *cr = cairo_create(img->surface);
        cairo_set_source_rgba(cr, ((rgb >> 16) & 0xFF) / 255.0, ((rgb >> 8) & 0xFF) / 255.0,
                              (rgb & 0xFF) / 255.0, 1.0);
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        if (img->hasDrawingTransform) cairo_set_matrix(cr, &img->drawingTransform);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill(cr);
        cairo_destroy(cr);
        return;
    }

    int x0 = std::max(x, 0), y0 = std::max(y, 0);
    int x1 = std::min(x + w, img->width), y1 = std::min(y + h, img->height);
    uint8_t r, g, b, a;
    for (int py = y0; py < y1; py++) {
        for (int px = x0; px < x1; px++) {
            surfaceRGBA(img->surface, px, py, &r, &g, &b, &a);
            uint8_t nr = (uint8_t)((rgb >> 16) & 0xFF) ^ (uint8_t)((img->xorColor >> 16) & 0xFF) ^ r;
            uint8_t ng = (uint8_t)((rgb >> 8) & 0xFF) ^ (uint8_t)((img->xorColor >> 8) & 0xFF) ^ g;
            uint8_t nb = (uint8_t)(rgb & 0xFF) ^ (uint8_t)(img->xorColor & 0xFF) ^ b;
            surfaceSetRGBA(img->surface, px, py, nr, ng, nb, 255);
        }
    }
}

void drawingPostMul(ImageData *img, const cairo_matrix_t &t)
{
    const cairo_matrix_t &m = img->drawingTransform;
    cairo_matrix_t r;
    r.xx = m.xx * t.xx + m.xy * t.yx;
    r.xy = m.xx * t.xy + m.xy * t.yy;
    r.yx = m.yx * t.xx + m.yy * t.yx;
    r.yy = m.yx * t.xy + m.yy * t.yy;
    r.x0 = m.xx * t.x0 + m.xy * t.y0 + m.x0;
    r.y0 = m.yx * t.x0 + m.yy * t.y0 + m.y0;
    img->drawingTransform = r;
    img->hasDrawingTransform = true;
}

cfvariant *nullResult()
{
    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

const cfvariant *structGet(const cfvariant *v, const char *key)
{
    if (!v || v->m_type != cfvariant::Struct || !v->m_struct) return nullptr;
    auto it = v->m_struct->find(string(key));
    return it != v->m_struct->end() ? &it->second : nullptr;
}

cfvariant *imageResult(ImageData *img)
{
    auto *ret = new cfvariant(cfvariant::Image);
    ret->m_image = img;
    return ret;
}

ImageData *imageClone(ImageData *src)
{
    auto *img = new ImageData;
    img->width = src->width;
    img->height = src->height;
    img->colormodel = src->colormodel;
    img->colormodelType = src->colormodelType;
    img->source = src->source;
    img->sourceFormat = src->sourceFormat;
    img->sourceBytes = src->sourceBytes;
    img->drawingColor = src->drawingColor;
    img->backgroundColor = src->backgroundColor;
    img->transparency = src->transparency;
    img->antialias = src->antialias;
    img->xorMode = src->xorMode;
    img->xorColor = src->xorColor;
    img->strokeWidth = src->strokeWidth;
    img->strokeCaps = src->strokeCaps;
    img->strokeJoins = src->strokeJoins;
    img->strokeMiterLimit = src->strokeMiterLimit;
    img->strokeDash = src->strokeDash;
    img->strokeDashPhase = src->strokeDashPhase;
    img->drawingTransform = src->drawingTransform;
    img->hasDrawingTransform = src->hasDrawingTransform;
    img->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, src->width, src->height);
    cairo_t *cr = cairo_create(img->surface);
    cairo_set_source_surface(cr, src->surface, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);
    return img;
}

void imageReplaceSurface(ImageData *img, cairo_surface_t *newSurface, int w, int h)
{
    if (img->surface) cairo_surface_destroy(img->surface);
    img->surface = newSurface;
    img->width = w;
    img->height = h;
    img->sourceBytes.clear(); // derived image: no raw source bytes
}

void imgPixel(const ImageData *img, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
    surfaceRGBA(img->surface, x, y, r, g, b, a);
}

void imgSetPixel(ImageData *dst, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    surfaceSetRGBA(dst->surface, x, y, r, g, b, a);
}

void imageTransform(ImageData *img,
                           const std::function<void(uint8_t&,uint8_t&,uint8_t&,uint8_t&)> &fn)
{
    uint8_t r, g, b, a;
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            imgPixel(img, x, y, &r, &g, &b, &a);
            fn(r, g, b, a);
            imgSetPixel(img, x, y, r, g, b, a);
        }
    }
}

uint32_t imgParseColor(const cfvariant *color)
{
    if (!color) return 0xFF000000u; // default black
    std::string s = toStdString(color);
    if (s.empty()) return 0xFF000000u;
    try {
        return parseDrawColor(s);
    } catch (...) {
        imageThrow("Application", "Invalid color.", "Verify your inputs. Invalid color.");
    }
    return 0xFF000000u;
}

void samplePixel(const ImageData *img, double fx, double fy, bool bilinear,
                        uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
    int w = img->width, h = img->height;
    if (!bilinear) {
        int ix = (int)fx, iy = (int)fy;
        if (ix < 0) ix = 0; if (ix >= w) ix = w - 1;
        if (iy < 0) iy = 0; if (iy >= h) iy = h - 1;
        imgPixel(img, ix, iy, r, g, b, a);
        return;
    }
    int x0 = (int)fx, y0 = (int)fy;
    double tx = fx - x0, ty = fy - y0;
    int x1 = x0 + 1, y1 = y0 + 1;
    if (x0 < 0) x0 = 0; if (x0 >= w) x0 = w - 1;
    if (x1 < 0) x1 = 0; if (x1 >= w) x1 = w - 1;
    if (y0 < 0) y0 = 0; if (y0 >= h) y0 = h - 1;
    if (y1 < 0) y1 = 0; if (y1 >= h) y1 = h - 1;
    uint8_t r00,g00,b00,a00, r01,g01,b01,a01, r10,g10,b10,a10, r11,g11,b11,a11;
    imgPixel(img, x0, y0, &r00,&g00,&b00,&a00);
    imgPixel(img, x1, y0, &r01,&g01,&b01,&a01);
    imgPixel(img, x0, y1, &r10,&g10,&b10,&a10);
    imgPixel(img, x1, y1, &r11,&g11,&b11,&a11);
    auto lerp = [&](int p00,int p01,int p10,int p11)->uint8_t {
        double top = p00 + (p01 - p00) * tx;
        double bot = p10 + (p11 - p10) * tx;
        return (uint8_t)(top + (bot - top) * ty + 0.5);
    };
    *r = lerp(r00,r01,r10,r11);
    *g = lerp(g00,g01,g10,g11);
    *b = lerp(b00,b01,b10,b11);
    *a = lerp(a00,a01,a10,a11);
}

std::string vToString(const cfvariant *v)
{
    if (!v) return "";
    webstrada::string tmp = const_cast<cfvariant*>(v)->toString();
    const char *d = tmp.constData();
    return d ? d : "";
}

std::string vToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool vToBool(const cfvariant *v)
{
    if (!v) return false;
    if (v->m_type == cfvariant::Boolean) return v->m_bool;
    std::string s = vToLower(vToString(v));
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    s = (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
    if (s.empty() || s == "false" || s == "no" || s == "off" || s == "0" || s == "null") return false;
    return true;
}

static cfvariant cfvariantFloat(double d)
{
    cfvariant v(cfvariant::Float);
    v.m_double = d;
    return v;
}

static cfvariant cfvariantBool(bool b)
{
    cfvariant v(cfvariant::Boolean);
    v.m_bool = b;
    return v;
}

void jpegAppSegments(const std::vector<std::byte> &bytes,
                            std::vector<std::vector<std::byte>> &app1,
                            std::vector<std::vector<std::byte>> &app13)
{
    if (bytes.size() < 2 || bytes[0] != (std::byte)0xFF || bytes[1] != (std::byte)0xD8) return;
    size_t i = 2;
    const size_t n = bytes.size();
    while (i + 1 < n) {
        if ((unsigned char)bytes[i] != 0xFF) { i++; continue; }
        unsigned char code = (unsigned char)bytes[i + 1];
        if (code == 0xD8 || code == 0xD9 || code == 0x01 ||
            (code >= 0xD0 && code <= 0xD7)) {
            i += 2;
            continue;
        }
        if (code == 0x00) { i += 2; continue; }
        if (i + 4 > n) return;
        size_t len = ((size_t)(unsigned char)bytes[i + 2] << 8) | (unsigned char)bytes[i + 3];
        if (len < 2 || i + 2 + len > n) return;
        std::vector<std::byte> payload;
        payload.reserve(len - 2);
        for (size_t k = i + 4; k < i + 2 + len; k++) payload.push_back(bytes[k]);
        if (code == 0xE1) app1.push_back(std::move(payload));
        else if (code == 0xED) app13.push_back(std::move(payload));
        i += 2 + len;
    }
}

int tiffTypeSize(int type)
{
    static const int sizes[13] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};
    return (type >= 1 && type <= 12) ? sizes[type] : 0;
}

std::string tiffAscii(const TiffBytes &r, size_t off, int count)
{
    int len = 0;
    while (len < count && (size_t)(off + len) < r.n && r.p[off + len] != 0) len++;
    return std::string((const char*)r.p + off, (size_t)len);
}

std::string rationalToString(long long num, long long den)
{
    return std::to_string(num) + "/" + std::to_string(den);
}

bool rationalIsInteger(long long num, long long den)
{
    return den == 1 || (den != 0 && num % den == 0) || (den == 0 && num == 0);
}

bool rationalTooComplex(long long num, long long den)
{
    long long m = std::min(den, num);
    return (((double)(m - 1)) / 5.0) + 2.0 > 1000.0;
}

void rationalSimplified(long long num, long long den, long long &snum, long long &sden)
{
    if (rationalTooComplex(num, den)) { snum = num; sden = den; return; }
    long long limit = std::min(den, num);
    for (long long i = 2; i <= limit; i++) {
        if ((i % 2 != 0 || i <= 2) && (i % 5 != 0 || i <= 5) &&
            den % i == 0 && num % i == 0) {
            snum = num / i; sden = den / i;
            return;
        }
    }
    snum = num; sden = den;
}

std::string javaDoubleToString(double d)
{
    if (std::isnan(d)) return "NaN";
    if (std::isinf(d)) return d > 0 ? "Infinity" : "-Infinity";
    if (d == (double)(long long)d && std::fabs(d) < 1e7) {
        return std::to_string((long long)d) + ".0";
    }
    char buf[64];
    for (int prec = 1; prec <= 17; prec++) {
        std::snprintf(buf, sizeof(buf), "%.*g", prec, d);
        double back = strtod(buf, nullptr);
        if (back == d) {
            // %.g may emit a bare "5" for integral doubles, but integral
            // values are handled above; exponent forms are kept as-is.
            return buf;
        }
    }
    std::snprintf(buf, sizeof(buf), "%.17g", d);
    return buf;
}

std::string rationalToSimpleString(long long num, long long den, bool allowDecimal)
{
    if (den == 0 && num != 0) return rationalToString(num, den);
    if (rationalIsInteger(num, den)) {
        long long iv = (long long)((double)num / (double)den);
        return std::to_string(iv);
    }
    if (num != 1 && den % num == 0) {
        return rationalToSimpleString(1, den / num, allowDecimal);
    }
    long long snum, sden;
    rationalSimplified(num, den, snum, sden);
    if (allowDecimal) {
        std::string s = javaDoubleToString((double)snum / (double)sden);
        if (s.length() < 5) return s;
    }
    return rationalToString(snum, sden);
}

const TiffValue *tiffGet(const TiffDir &dir, int tag)
{
    for (const auto &t : dir.tags) {
        if (t.tag == tag) return &t.val;
        if (t.tag > tag) break;
    }
    return nullptr;
}

long long rationalIntValue(long long num, long long den)
{
    return (long long)((double)num / (double)den);
}

std::string tiffGetString(const TiffDir &dir, int tag)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v) return "";
    switch (v->kind) {
    case TiffValue::STRING: return v->str;
    case TiffValue::INT:
    case TiffValue::LONG: return std::to_string(v->i);
    case TiffValue::RATIONAL: return rationalToSimpleString(v->num, v->den, true);
    case TiffValue::RATIONAL_ARRAY: {
        std::string s;
        for (size_t i = 0; i < v->rnum.size(); i++) {
            if (i) s += ' ';
            s += rationalToString(v->rnum[i], v->rden[i]);
        }
        return s;
    }
    case TiffValue::INT_ARRAY:
    case TiffValue::LONG_ARRAY:
    case TiffValue::SHORT_ARRAY: {
        std::string s;
        for (size_t i = 0; i < v->iarr.size(); i++) {
            if (i) s += ' ';
            s += std::to_string(v->iarr[i]);
        }
        return s;
    }
    case TiffValue::BYTE_ARRAY: {
        std::string s;
        for (size_t i = 0; i < v->bytes.size(); i++) {
            if (i) s += ' ';
            s += std::to_string((int)(int8_t)v->bytes[i]);
        }
        return s;
    }
    case TiffValue::FLOAT_ARRAY: {
        std::string s;
        for (size_t i = 0; i < v->farr.size(); i++) {
            if (i) s += ' ';
            s += javaDoubleToString(v->farr[i]);
        }
        return s;
    }
    case TiffValue::DOUBLE_ARRAY: {
        std::string s;
        for (size_t i = 0; i < v->darr.size(); i++) {
            if (i) s += ' ';
            s += javaDoubleToString(v->darr[i]);
        }
        return s;
    }
    default: return "";
    }
}

bool javaParseInt(const std::string &s, long long &out)
{
    if (s.empty()) return false;
    size_t i = 0;
    bool neg = false;
    if (s[0] == '+' || s[0] == '-') { neg = s[0] == '-'; i = 1; }
    if (i >= s.size()) return false;
    long long v = 0;
    for (; i < s.size(); i++) {
        if (s[i] < '0' || s[i] > '9') return false;
        v = v * 10 + (s[i] - '0');
    }
    out = neg ? -v : v;
    return true;
}

long long tiffGetInteger(const TiffDir &dir, int tag)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v) return 0;
    switch (v->kind) {
    case TiffValue::INT:
    case TiffValue::LONG: return v->i;
    case TiffValue::STRING: {
        long long out;
        if (javaParseInt(v->str, out)) return out;
        long long j = 0;
        for (unsigned char b : v->str) j = (j << 8) | (long long)(b & 0xFF);
        return j;
    }
    case TiffValue::RATIONAL_ARRAY:
        if (v->rnum.size() == 1) return rationalIntValue(v->rnum[0], v->rden[0]);
        return 0;
    case TiffValue::BYTE_ARRAY:
        if (v->bytes.size() == 1) return (long long)(int8_t)v->bytes[0];
        return 0;
    case TiffValue::INT_ARRAY:
    case TiffValue::SHORT_ARRAY:
        if (v->iarr.size() == 1) return v->iarr[0];
        return 0;
    case TiffValue::RATIONAL:
        return rationalIntValue(v->num, v->den);
    default: return 0;
    }
}

bool tiffGetRational(const TiffDir &dir, int tag, long long &num, long long &den)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v) return false;
    switch (v->kind) {
    case TiffValue::RATIONAL: num = v->num; den = v->den; return true;
    case TiffValue::INT:
    case TiffValue::LONG: num = v->i; den = 1; return true;
    default: return false;
    }
}

bool tiffGetRationalArray(const TiffDir &dir, int tag, std::vector<long long> &num, std::vector<long long> &den)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v || v->kind != TiffValue::RATIONAL_ARRAY) return false;
    num = v->rnum;
    den = v->rden;
    return true;
}

std::vector<long long> tiffGetIntArray(const TiffDir &dir, int tag)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v) return {};
    switch (v->kind) {
    case TiffValue::INT_ARRAY:
    case TiffValue::LONG_ARRAY:
    case TiffValue::SHORT_ARRAY:
        return v->iarr;
    case TiffValue::RATIONAL_ARRAY: {
        std::vector<long long> out;
        for (size_t i = 0; i < v->rnum.size(); i++)
            out.push_back(rationalIntValue(v->rnum[i], v->rden[i]));
        return out;
    }
    case TiffValue::BYTE_ARRAY: {
        std::vector<long long> out;
        for (unsigned char b : v->bytes) out.push_back((long long)(int8_t)b);
        return out;
    }
    case TiffValue::STRING: {
        std::vector<long long> out;
        for (unsigned char c : v->str) out.push_back((long long)c);
        return out;
    }
    case TiffValue::INT:
    case TiffValue::LONG:
        return {v->i};
    default: return {};
    }
}

std::vector<unsigned char> tiffGetByteArray(const TiffDir &dir, int tag)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v) return {};
    switch (v->kind) {
    case TiffValue::BYTE_ARRAY: return v->bytes;
    case TiffValue::RATIONAL_ARRAY: {
        std::vector<unsigned char> out;
        for (size_t i = 0; i < v->rnum.size(); i++)
            out.push_back((unsigned char)(int8_t)rationalIntValue(v->rnum[i], v->rden[i]));
        return out;
    }
    case TiffValue::INT_ARRAY:
    case TiffValue::LONG_ARRAY:
    case TiffValue::SHORT_ARRAY: {
        std::vector<unsigned char> out;
        for (long long x : v->iarr) out.push_back((unsigned char)(int8_t)x);
        return out;
    }
    case TiffValue::STRING: {
        std::vector<unsigned char> out;
        for (unsigned char c : v->str) out.push_back(c);
        return out;
    }
    case TiffValue::INT:
    case TiffValue::LONG:
        return {(unsigned char)(int8_t)v->i};
    default: return {};
    }
}

double tiffGetDouble(const TiffDir &dir, int tag)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v) return 0.0;
    switch (v->kind) {
    case TiffValue::STRING: {
        const char *s = v->str.c_str();
        char *end = nullptr;
        double d = strtod(s, &end);
        if (end && *end == 0) return d;
        return 0.0;
    }
    case TiffValue::INT:
    case TiffValue::LONG: return (double)v->i;
    case TiffValue::DOUBLE_ARRAY:
        if (v->darr.size() == 1) return v->darr[0];
        break;
    case TiffValue::FLOAT_ARRAY:
        if (v->farr.size() == 1) return v->farr[0];
        break;
    case TiffValue::RATIONAL:
        return (double)v->num / (double)v->den;
    default: break;
    }
    return 0.0;
}

float tiffGetFloat(const TiffDir &dir, int tag)
{
    return (float)tiffGetDouble(dir, tag);
}

const std::map<int, std::string> &exifTagNames()
{
    static const std::map<int, std::string> m = {
        {1, "Interoperability Index"}, {2, "Interoperability Version"},
        {254, "New Subfile Type"}, {255, "Subfile Type"},
        {256, "Image Width"}, {257, "Image Height"}, {258, "Bits Per Sample"},
        {259, "Compression"}, {262, "Photometric Interpretation"},
        {263, "Thresholding"}, {266, "Fill Order"}, {269, "Document Name"},
        {270, "Image Description"}, {271, "Make"}, {272, "Model"},
        {273, "Strip Offsets"}, {274, "Orientation"}, {277, "Samples Per Pixel"},
        {278, "Rows Per Strip"}, {279, "Strip Byte Counts"},
        {280, "Minimum sample value"}, {281, "Maximum sample value"},
        {282, "X Resolution"}, {283, "Y Resolution"}, {284, "Planar Configuration"},
        {285, "Page Name"}, {296, "Resolution Unit"}, {301, "Transfer Function"},
        {305, "Software"}, {306, "Date/Time"}, {315, "Artist"},
        {316, "Host Computer"}, {317, "Predictor"}, {318, "White Point"},
        {319, "Primary Chromaticities"}, {322, "Tile Width"}, {323, "Tile Length"},
        {324, "Tile Offsets"}, {325, "Tile Byte Counts"},
        {330, "Sub IFD Pointer(s)"}, {342, "Transfer Range"},
        {347, "JPEG Tables"}, {512, "JPEG Proc"},
        {529, "YCbCr Coefficients"}, {530, "YCbCr Sub-Sampling"},
        {531, "YCbCr Positioning"}, {532, "Reference Black/White"},
        {4096, "Related Image File Format"}, {4097, "Related Image Width"},
        {4098, "Related Image Height"}, {18246, "Rating"},
        {33421, "CFA Repeat Pattern Dim"}, {33422, "CFA Pattern"},
        {33423, "Battery Level"}, {33432, "Copyright"},
        {33434, "Exposure Time"}, {33437, "F-Number"}, {33723, "IPTC/NAA"},
        {34675, "Inter Color Profile"}, {34850, "Exposure Program"},
        {34852, "Spectral Sensitivity"}, {34855, "ISO Speed Ratings"},
        {34856, "Opto-electric Conversion Function (OECF)"},
        {34857, "Interlace"}, {34858, "Time Zone Offset"},
        {34859, "Self Timer Mode"}, {34864, "Sensitivity Type"},
        {34865, "Standard Output Sensitivity"}, {34866, "Recommended Exposure Index"},
        {36864, "Exif Version"}, {36867, "Date/Time Original"},
        {36868, "Date/Time Digitized"}, {37121, "Components Configuration"},
        {37122, "Compressed Bits Per Pixel"}, {37377, "Shutter Speed Value"},
        {37378, "Aperture Value"}, {37379, "Brightness Value"},
        {37380, "Exposure Bias Value"}, {37381, "Max Aperture Value"},
        {37382, "Subject Distance"}, {37383, "Metering Mode"},
        {37384, "Light Source"}, {37385, "Flash"}, {37386, "Focal Length"},
        {37387, "Flash Energy"}, {37388, "Spatial Frequency Response"},
        {37389, "Noise"}, {37390, "Focal Plane X Resolution"},
        {37391, "Focal Plane Y Resolution"}, {37393, "Image Number"},
        {37394, "Security Classification"}, {37395, "Image History"},
        {37396, "Subject Location"}, {37397, "Exposure Index"},
        {37398, "TIFF/EP Standard ID"}, {37500, "Makernote"},
        {37510, "User Comment"}, {37520, "Sub-Sec Time"},
        {37521, "Sub-Sec Time Original"}, {37522, "Sub-Sec Time Digitized"},
        {40091, "Windows XP Title"}, {40092, "Windows XP Comment"},
        {40093, "Windows XP Author"}, {40094, "Windows XP Keywords"},
        {40095, "Windows XP Subject"}, {40960, "FlashPix Version"},
        {40961, "Color Space"}, {40962, "Exif Image Width"},
        {40963, "Exif Image Height"}, {40964, "Related Sound File"},
        {41483, "Flash Energy"}, {41484, "Spatial Frequency Response"},
        {41486, "Focal Plane X Resolution"}, {41487, "Focal Plane Y Resolution"},
        {41488, "Focal Plane Resolution Unit"}, {41492, "Subject Location"},
        {41493, "Exposure Index"}, {41495, "Sensing Method"},
        {41728, "File Source"}, {41729, "Scene Type"}, {41730, "CFA Pattern"},
        {41985, "Custom Rendered"}, {41986, "Exposure Mode"},
        {41987, "White Balance Mode"}, {41988, "Digital Zoom Ratio"},
        {41989, "Focal Length 35"}, {41990, "Scene Capture Type"},
        {41991, "Gain Control"}, {41992, "Contrast"}, {41993, "Saturation"},
        {41994, "Sharpness"}, {41995, "Device Setting Description"},
        {41996, "Subject Distance Range"}, {42016, "Unique Image ID"},
        {42032, "Camera Owner Name"}, {42033, "Body Serial Number"},
        {42034, "Lens Specification"}, {42035, "Lens Make"},
        {42036, "Lens Model"}, {42037, "Lens Serial Number"},
        {42240, "Gamma"}, {50341, "Print IM"},
        {50898, "Panasonic Title"}, {50899, "Panasonic Title (2)"},
        {59932, "Padding"}, {65002, "Lens"},
    };
    return m;
}

const std::map<int, std::string> &gpsTagNames()
{
    static const std::map<int, std::string> m = {
        {0, "GPS Version ID"}, {1, "GPS Latitude Ref"}, {2, "GPS Latitude"},
        {3, "GPS Longitude Ref"}, {4, "GPS Longitude"}, {5, "GPS Altitude Ref"},
        {6, "GPS Altitude"}, {7, "GPS Time-Stamp"}, {8, "GPS Satellites"},
        {9, "GPS Status"}, {10, "GPS Measure Mode"}, {11, "GPS DOP"},
        {12, "GPS Speed Ref"}, {13, "GPS Speed"}, {14, "GPS Track Ref"},
        {15, "GPS Track"}, {16, "GPS Img Direction Ref"},
        {17, "GPS Img Direction"}, {18, "GPS Map Datum"},
        {19, "GPS Dest Latitude Ref"}, {20, "GPS Dest Latitude"},
        {21, "GPS Dest Longitude Ref"}, {22, "GPS Dest Longitude"},
        {23, "GPS Dest Bearing Ref"}, {24, "GPS Dest Bearing"},
        {25, "GPS Dest Distance Ref"}, {26, "GPS Dest Distance"},
        {27, "GPS Processing Method"}, {28, "GPS Area Information"},
        {29, "GPS Date Stamp"}, {30, "GPS Differential"},
    };
    return m;
}

std::string tagName(const std::map<int, std::string> &map, int tag)
{
    auto it = map.find(tag);
    if (it != map.end()) return it->second;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Unknown tag (0x%04x)", tag);
    return buf;
}

int tiffPointerKind(int tag, int curKind)
{
    if (curKind == 0) {
        if (tag == 0x8769) return 1; // SubIFD
        if (tag == 0x8825) return 2; // GPS
    } else if (curKind == 1) {
        if (tag == 0xA005) return 3; // Interop
    }
    return -1;
}

bool tiffHasFollower(int curKind)
{
    return curKind == 0 || curKind == 4;
}

void tiffSetValue(TiffDir &dir, int tag, int type, int count, const TiffBytes &r, size_t off)
{
    TiffTag t;
    t.tag = tag;
    TiffValue &v = t.val;
    switch (type) {
    case 1: // BYTE
        if (count == 1) { v.kind = TiffValue::INT; v.i = r.u8(off); }
        else {
            v.kind = TiffValue::BYTE_ARRAY;
            for (int i = 0; i < count && (size_t)(off + i) < r.n; i++) v.bytes.push_back(r.p[off + i]);
        }
        break;
    case 2: // ASCII
        v.kind = TiffValue::STRING;
        v.str = tiffAscii(r, off, count);
        break;
    case 3: // SHORT
        if (count == 1) { v.kind = TiffValue::INT; v.i = r.u16(off); }
        else {
            v.kind = TiffValue::INT_ARRAY;
            for (int i = 0; i < count && (size_t)(off + 2 + 2 * i) <= r.n; i++) v.iarr.push_back(r.u16(off + 2 * i));
        }
        break;
    case 4: // LONG
        if (count == 1) { v.kind = TiffValue::LONG; v.i = r.u32(off); }
        else {
            v.kind = TiffValue::LONG_ARRAY;
            for (int i = 0; i < count && (size_t)(off + 4 + 4 * i) <= r.n; i++) v.iarr.push_back(r.u32(off + 4 * i));
        }
        break;
    case 5: // RATIONAL
        if (count == 1) { v.kind = TiffValue::RATIONAL; v.num = r.u32(off); v.den = r.u32(off + 4); }
        else if (count > 1) {
            v.kind = TiffValue::RATIONAL_ARRAY;
            for (int i = 0; i < count && (size_t)(off + 8 + 8 * i) <= r.n; i++) {
                v.rnum.push_back(r.u32(off + 8 * i));
                v.rden.push_back(r.u32(off + 4 + 8 * i));
            }
        }
        break;
    case 6: // SBYTE
        if (count == 1) { v.kind = TiffValue::INT; v.i = (int8_t)r.p[off]; }
        else {
            v.kind = TiffValue::BYTE_ARRAY;
            for (int i = 0; i < count && (size_t)(off + i) < r.n; i++) v.bytes.push_back(r.p[off + i]);
        }
        break;
    case 7: // UNDEFINED
        v.kind = TiffValue::BYTE_ARRAY;
        for (int i = 0; i < count && (size_t)(off + i) < r.n; i++) v.bytes.push_back(r.p[off + i]);
        break;
    case 8: // SSHORT
        if (count == 1) { v.kind = TiffValue::INT; v.i = r.s16(off); }
        else {
            v.kind = TiffValue::SHORT_ARRAY;
            for (int i = 0; i < count && (size_t)(off + 2 + 2 * i) <= r.n; i++) v.iarr.push_back(r.s16(off + 2 * i));
        }
        break;
    case 9: // SLONG
        if (count == 1) { v.kind = TiffValue::INT; v.i = r.s32(off); }
        else {
            v.kind = TiffValue::INT_ARRAY;
            for (int i = 0; i < count && (size_t)(off + 4 + 4 * i) <= r.n; i++) v.iarr.push_back(r.s32(off + 4 * i));
        }
        break;
    case 10: // SRATIONAL
        if (count == 1) { v.kind = TiffValue::RATIONAL; v.num = r.s32(off); v.den = r.s32(off + 4); }
        else if (count > 1) {
            v.kind = TiffValue::RATIONAL_ARRAY;
            for (int i = 0; i < count && (size_t)(off + 8 + 8 * i) <= r.n; i++) {
                v.rnum.push_back(r.s32(off + 8 * i));
                v.rden.push_back(r.s32(off + 4 + 8 * i));
            }
        }
        break;
    case 11: // FLOAT
        if (count == 1) { v.kind = TiffValue::FLOAT_ARRAY; v.darr.clear(); v.farr.clear(); v.kind = TiffValue::FLOAT_ARRAY; v.farr.push_back(r.f32(off)); }
        else {
            v.kind = TiffValue::FLOAT_ARRAY;
            for (int i = 0; i < count && (size_t)(off + 4 + 4 * i) <= r.n; i++) v.farr.push_back(r.f32(off + 4 * i));
        }
        break;
    case 12: // DOUBLE
        if (count == 1) { v.kind = TiffValue::DOUBLE_ARRAY; v.darr.push_back(r.f64(off)); }
        else {
            v.kind = TiffValue::DOUBLE_ARRAY;
            for (int i = 0; i < count && (size_t)(off + 8 + 8 * i) <= r.n; i++) v.darr.push_back(r.f64(off + 8 * i));
        }
        break;
    default:
        return; // unknown format code
    }
    dir.tags.push_back(t);
}

void parseTiffIfd(const TiffBytes &r, size_t base, size_t ifdOff,
                         int kind, std::vector<TiffDir> &dirs, std::vector<size_t> &visited)
{
    if (std::find(visited.begin(), visited.end(), ifdOff) != visited.end()) return;
    visited.push_back(ifdOff);
    if (ifdOff >= r.n) return;

    int count = r.u16(ifdOff);
    if (2 + 12 * count + 4 + ifdOff > r.n) return; // illegitimately sized IFD

    // Ensure the target directory slot exists.
    while ((int)dirs.size() <= kind) dirs.push_back(TiffDir{0, {}});
    TiffDir &dir = dirs[kind];
    dir.kind = kind;

    int errors = 0;
    for (int i = 0; i < count; i++) {
        size_t e = ifdOff + 2 + 12 * i;
        int tag = r.u16(e);
        int type = r.u16(e + 2);
        int32_t compCount = r.s32(e + 4);
        if (compCount < 0) { errors++; if (errors > 5) return; continue; }
        int compSize = tiffTypeSize(type);
        if (compSize == 0) { errors++; if (errors > 5) return; continue; }
        long long total = (long long)compCount * compSize;
        size_t dataOff;
        if (total > 4) {
            int32_t ptr = r.s32(e + 8);
            if (ptr < 0 || base + (size_t)ptr + (size_t)total > r.n) {
                errors++; if (errors > 5) return; continue;
            }
            dataOff = base + (size_t)ptr;
        } else {
            dataOff = e + 8;
        }

        int ptrKind = tiffPointerKind(tag, kind);
        if (total == 4 && ptrKind >= 0 && kind != 4) {
            int32_t ptr = r.s32(dataOff);
            if (ptr >= 0 && base + (size_t)ptr < r.n)
                parseTiffIfd(r, base, base + (size_t)ptr, ptrKind, dirs, visited);
            continue;
        }
        tiffSetValue(dir, tag, type, compCount, r, dataOff);
    }

    // Follower IFD (thumbnail) chain.
    if (2 + 12 * count + 4 <= r.n) {
        int32_t next = r.s32(ifdOff + 2 + 12 * count);
        if (next != 0) {
            size_t nextAbs = base + (size_t)next;
            if (nextAbs >= r.n) return;
            if (nextAbs < ifdOff) return;
            if (tiffHasFollower(kind)) {
                int nk = (kind == 0) ? 4 : 4; // both route to Thumbnail
                parseTiffIfd(r, base, nextAbs, nk, dirs, visited);
            }
        }
    }
}

bool parseTiff(const std::vector<std::byte> &bytes, size_t tiffStart,
                      std::vector<TiffDir> &dirs)
{
    if (tiffStart + 8 > bytes.size()) return false;
    const uint8_t *p = (const uint8_t*)bytes.data() + tiffStart;
    if (bytes.size() - tiffStart < 8) return false;
    bool be;
    if (p[0] == 'I' && p[1] == 'I') be = false;
    else if (p[0] == 'M' && p[1] == 'M') be = true;
    else return false;
    TiffBytes r;
    r.p = (const uint8_t*)bytes.data();
    r.n = bytes.size();
    r.be = be;
    uint16_t magic = r.u16(tiffStart + 2);
    if (magic != 42 && magic != 20306 && magic != 21330 && magic != 85) return false;
    int32_t first = r.s32(tiffStart + 4);
    size_t ifdOff;
    if (first < 0 || (size_t)first + tiffStart >= r.n - 1) {
        ifdOff = tiffStart + 8;
    } else {
        ifdOff = tiffStart + (size_t)first;
    }
    std::vector<size_t> visited;
    parseTiffIfd(r, tiffStart, ifdOff, 0, dirs, visited);
    return !dirs.empty();
}

std::string decimalFormat00_00(double d)
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f", d);
    return buf;
}

std::string javaDecimalFormat(int minFrac, int maxFrac, double d)
{
    if (maxFrac <= 0) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f", d);
        return buf;
    }
    double p = std::pow(10.0, maxFrac);
    double r = std::nearbyint(d * p) / p;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", maxFrac, r);
    std::string s = buf;
    size_t dot = s.find('.');
    if (dot == std::string::npos) return s;
    size_t lastNonZero = s.find_last_not_of('0');
    size_t minPos = dot + minFrac;
    size_t e = (lastNonZero < minPos) ? minPos : lastNonZero;
    if (e >= s.size()) e = s.size() - 1;
    s = s.substr(0, e + 1);
    // When no mandatory fractional digit was required, a value with no
    // non-zero fraction must not keep the decimal point (Java "0.##" -> "51").
    if (minFrac == 0 && !s.empty() && s.back() == '.') s.pop_back();
    return s;
}

double apertureToFStop(double d)
{
    return std::pow(std::sqrt(2.0), d);
}

std::string indexedDescription(const TiffDir &dir, int tag, int start,
                                      const std::vector<std::string> &values)
{
    long long iv = tiffGetInteger(dir, tag);
    if (iv == 0 && tiffGet(dir, tag) == nullptr) return "";
    long long idx = iv - start;
    if (idx < 0 || idx >= (long long)values.size() || values[idx].empty())
        return "Unknown (" + std::to_string(iv) + ")";
    return values[idx];
}

std::string versionBytesDescription(const TiffDir &dir, int tag, int insertIndex)
{
    std::vector<long long> arr = tiffGetIntArray(dir, tag);
    if (arr.empty()) return "";
    std::string sb;
    for (size_t i = 0; i < 4 && i < arr.size(); i++) {
        if ((int)i == insertIndex) sb += '.';
        char c = (char)arr[i];
        if (c < '0') c = (char)(c + '0');
        if (i != 0 || c != '0') sb += c;
    }
    return sb;
}

std::string exifDescription(const TiffDir &dir, int tag);

std::string tagDescriptorFallback(const TiffDir &dir, int tag, const TiffValue *v)
{
    if (!v) return "";
    const char *compType = "";
    size_t len = 0;
    switch (v->kind) {
    case TiffValue::INT_ARRAY: case TiffValue::SHORT_ARRAY:
        compType = "int"; len = v->iarr.size(); break;
    case TiffValue::LONG_ARRAY:
        compType = "long"; len = v->iarr.size(); break;
    case TiffValue::BYTE_ARRAY:
        compType = "byte"; len = v->bytes.size(); break;
    case TiffValue::FLOAT_ARRAY:
        compType = "float"; len = v->farr.size(); break;
    case TiffValue::DOUBLE_ARRAY:
        compType = "double"; len = v->darr.size(); break;
    case TiffValue::RATIONAL_ARRAY:
        compType = "com.drew.lang.Rational"; len = v->rnum.size(); break;
    default:
        return tiffGetString(dir, tag);
    }
    if (len > 16) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "[%zu %s%s]", len, compType, len == 1 ? "" : "s");
        return buf;
    }
    return tiffGetString(dir, tag);
}

std::string exifDescription(const TiffDir &dir, int tag)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v) return "";
    switch (tag) {
    case 1: {
        std::string s = tiffGetString(dir, 1);
        std::string t = s;
        size_t b = t.find_first_not_of(" \t");
        if (b != std::string::npos) t = t.substr(b);
        else t.clear();
        return (t == "R98" || t == "r98") ? "Recommended Exif Interoperability Rules (ExifR98)" : "Unknown (" + s + ")";
    }
    case 2: return versionBytesDescription(dir, 2, 2);
    case 254: return indexedDescription(dir, 254, 1, {"Full-resolution image", "Reduced-resolution image", "Single page of multi-page reduced-resolution image", "Transparency mask", "Transparency mask of reduced-resolution image", "Transparency mask of multi-page image", "Transparency mask of reduced-resolution multi-page image"});
    case 255: return indexedDescription(dir, 255, 1, {"Full-resolution image", "Reduced-resolution image", "Single page of multi-page image"});
    case 256: { std::string s = tiffGetString(dir, 256); return s.empty() ? "" : s + " pixels"; }
    case 257: { std::string s = tiffGetString(dir, 257); return s.empty() ? "" : s + " pixels"; }
    case 258: { std::string s = tiffGetString(dir, 258); return s.empty() ? "" : s + " bits/component/pixel"; }
    case 259: {
        long long iv = tiffGetInteger(dir, 259);
        if (!tiffGet(dir, 259)) return "";
        static const std::map<long long, std::string> comp = {
            {1, "Uncompressed"}, {2, "CCITT 1D"}, {3, "T4/Group 3 Fax"},
            {4, "T6/Group 4 Fax"}, {5, "LZW"}, {6, "JPEG (old-style)"},
            {7, "JPEG"}, {8, "Adobe Deflate"}, {9, "JBIG B&W"}, {10, "JBIG Color"},
            {99, "JPEG"}, {262, "Kodak 262"}, {32766, "Next"},
            {32767, "Sony ARW Compressed"}, {32769, "Packed RAW"},
            {32770, "Samsung SRW Compressed"}, {32771, "CCIRLEW"},
            {32772, "Samsung SRW Compressed 2"}, {32773, "PackBits"},
            {32809, "Thunderscan"}, {32867, "Kodak KDC Compressed"},
            {32895, "IT8CTPAD"}, {32896, "IT8LW"}, {32897, "IT8MP"},
            {32898, "IT8BL"}, {32908, "PixarFilm"}, {32909, "PixarLog"},
            {32946, "Deflate"}, {32947, "DCS"}, {34661, "JBIG"},
            {34676, "SGILog"}, {34677, "SGILog24"}, {34712, "JPEG 2000"},
            {34713, "Nikon NEF Compressed"}, {34715, "JBIG2 TIFF FX"},
            {34718, "Microsoft Document Imaging (MDI) Binary Level Codec"},
            {34719, "Microsoft Document Imaging (MDI) Progressive Transform Codec"},
            {34720, "Microsoft Document Imaging (MDI) Vector"},
            {34892, "Lossy JPEG"}, {65000, "Kodak DCR Compressed"},
            {65535, "Pentax PEF Compressed"},
        };
        auto it = comp.find(iv);
        return it != comp.end() ? it->second : "Unknown (" + std::to_string(iv) + ")";
    }
    case 262: {
        long long iv = tiffGetInteger(dir, 262);
        if (!tiffGet(dir, 262)) return "";
        switch (iv) {
        case 0: return "WhiteIsZero"; case 1: return "BlackIsZero";
        case 2: return "RGB"; case 3: return "RGB Palette";
        case 4: return "Transparency Mask"; case 5: return "CMYK";
        case 6: return "YCbCr"; case 8: return "CIELab"; case 9: return "ICCLab";
        case 10: return "ITULab"; case 32803: return "Color Filter Array";
        case 32844: return "Pixar LogL"; case 32845: return "Pixar LogLuv";
        case 32892: return "Linear Raw";
        default: return "Unknown colour space";
        }
    }
    case 263: return indexedDescription(dir, 263, 1, {"No dithering or halftoning", "Ordered dither or halftone", "Randomized dither"});
    case 266: return indexedDescription(dir, 266, 1, {"Normal", "Reversed"});
    case 274: return indexedDescription(dir, 274, 1, {"Top, left side (Horizontal / normal)", "Top, right side (Mirror horizontal)", "Bottom, right side (Rotate 180)", "Bottom, left side (Mirror vertical)", "Left side, top (Mirror horizontal and rotate 270 CW)", "Right side, top (Rotate 90 CW)", "Right side, bottom (Mirror horizontal and rotate 90 CW)", "Left side, bottom (Rotate 270 CW)"});
    case 277: { std::string s = tiffGetString(dir, 277); return s.empty() ? "" : s + " samples/pixel"; }
    case 278: { std::string s = tiffGetString(dir, 278); return s.empty() ? "" : s + " rows/strip"; }
    case 279: { std::string s = tiffGetString(dir, 279); return s.empty() ? "" : s + " bytes"; }
    case 282: case 283: {
        long long num, den;
        if (!tiffGetRational(dir, tag, num, den)) return "";
        std::string res = indexedDescription(dir, 296, 1, {"(No unit)", "Inch", "cm"});
        std::string unit = res.empty() ? "unit" : vToLower(res);
        return rationalToSimpleString(num, den, true) + " dots per " + unit;
    }
    case 284: return indexedDescription(dir, 284, 1, {"Chunky (contiguous for each subsampling pixel)", "Separate (Y-plane/Cb-plane/Cr-plane format)"});
    case 296: return indexedDescription(dir, 296, 1, {"(No unit)", "Inch", "cm"});
    case 512: {
        long long iv = tiffGetInteger(dir, 512);
        if (!tiffGet(dir, 512)) return "";
        switch (iv) { case 1: return "Baseline"; case 14: return "Lossless"; default: return "Unknown (" + std::to_string(iv) + ")"; }
    }
    case 530: {
        std::vector<long long> a = tiffGetIntArray(dir, 530);
        if (a.size() < 2) return "";
        if (a[0] == 2 && a[1] == 1) return "YCbCr4:2:2";
        return (a[0] == 2 && a[1] == 2) ? "YCbCr4:2:0" : "(Unknown)";
    }
    case 531: return indexedDescription(dir, 531, 1, {"Center of pixel array", "Datum point"});
    case 532: {
        std::vector<long long> a = tiffGetIntArray(dir, 532);
        if (a.size() < 6) return "";
        char buf[128];
        std::snprintf(buf, sizeof(buf), "[%lld,%lld,%lld] [%lld,%lld,%lld]",
                      (long long)a[0], (long long)a[2], (long long)a[4],
                      (long long)a[1], (long long)a[3], (long long)a[5]);
        return buf;
    }
    case 33434: { std::string s = tiffGetString(dir, 33434); return s.empty() ? "" : s + " sec"; }
    case 33437: {
        long long num, den;
        if (!tiffGetRational(dir, 33437, num, den)) return "";
        return "f/" + javaDecimalFormat(1, 1, (double)num / (double)den);
    }
    case 34850: return indexedDescription(dir, 34850, 1, {"Manual control", "Program normal", "Aperture priority", "Shutter priority", "Program creative (slow program)", "Program action (high-speed program)", "Portrait mode", "Landscape mode"});
    case 34855: {
        if (!tiffGet(dir, 34855)) return "";
        return std::to_string(tiffGetInteger(dir, 34855));
    }
    case 34864: return indexedDescription(dir, 34864, 0, {"Unknown", "Standard Output Sensitivity", "Recommended Exposure Index", "ISO Speed", "Standard Output Sensitivity and Recommended Exposure Index", "Standard Output Sensitivity and ISO Speed", "Recommended Exposure Index and ISO Speed", "Standard Output Sensitivity, Recommended Exposure Index and ISO Speed"});
    case 36864: return versionBytesDescription(dir, 36864, 2);
    case 37121: {
        std::vector<long long> a = tiffGetIntArray(dir, 37121);
        if (a.empty()) return "";
        static const char *names[] = {"", "Y", "Cb", "Cr", "R", "G", "B"};
        std::string sb;
        for (size_t i = 0; i < 4 && i < a.size(); i++) {
            long long x = a[i];
            if (x > 0 && x < 7) sb += names[x];
        }
        return sb;
    }
    case 37122: {
        long long num, den;
        if (!tiffGetRational(dir, 37122, num, den)) return "";
        std::string s = rationalToSimpleString(num, den, true);
        return (rationalIsInteger(num, den) && num / den == 1) ? s + " bit/pixel" : s + " bits/pixel";
    }
    case 37377: {
        float f = tiffGetFloat(dir, 37377);
        if (!tiffGet(dir, 37377)) return "";
        if (f <= 1.0f) {
            double v = 1.0 / std::exp((double)f * std::log(2.0));
            double r = std::round(v * 10.0) / 10.0;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%g", r);
            return std::string(buf) + " sec";
        }
        long long iv = (long long)std::exp((double)f * std::log(2.0));
        return "1/" + std::to_string(iv) + " sec";
    }
    case 37378: {
        double d = tiffGetDouble(dir, 37378);
        if (!tiffGet(dir, 37378)) return "";
        return "f/" + javaDecimalFormat(1, 1, apertureToFStop(d));
    }
    case 37380: {
        long long num, den;
        if (!tiffGetRational(dir, 37380, num, den)) return "";
        return rationalToSimpleString(num, den, true) + " EV";
    }
    case 37381: {
        double d = tiffGetDouble(dir, 37381);
        if (!tiffGet(dir, 37381)) return "";
        return "f/" + javaDecimalFormat(1, 1, apertureToFStop(d));
    }
    case 37382: {
        long long num, den;
        if (!tiffGetRational(dir, 37382, num, den)) return "";
        return javaDecimalFormat(1, 3, (double)num / (double)den) + " metres";
    }
    case 37383: {
        long long iv = tiffGetInteger(dir, 37383);
        if (!tiffGet(dir, 37383)) return "";
        switch (iv) {
        case 0: return "Unknown"; case 1: return "Average";
        case 2: return "Center weighted average"; case 3: return "Spot";
        case 4: return "Multi-spot"; case 5: return "Multi-segment";
        case 6: return "Partial"; case 255: return "(Other)";
        default: return "Unknown (" + std::to_string(iv) + ")";
        }
    }
    case 37384: {
        long long iv = tiffGetInteger(dir, 37384);
        if (!tiffGet(dir, 37384)) return "";
        switch (iv) {
        case 0: return "Unknown"; case 1: return "Daylight"; case 2: return "Florescent";
        case 3: return "Tungsten"; case 4: return "Flash"; case 9: return "Fine Weather";
        case 10: return "Cloudy"; case 11: return "Shade"; case 12: return "Daylight Flourescent";
        case 13: return "Day White Flourescent"; case 14: return "Cool White Flourescent";
        case 15: return "White Flourescent"; case 16: return "Warm White Flourescent";
        case 17: return "Standard light"; case 18: return "Standard light (B)";
        case 19: return "Standard light (C)"; case 20: return "D55"; case 21: return "D65";
        case 22: return "D75"; case 23: return "D50"; case 24: return "Studio Tungsten";
        case 255: return "(Other)";
        default: return "Unknown (" + std::to_string(iv) + ")";
        }
    }
    case 37385: {
        long long iv = tiffGetInteger(dir, 37385);
        if (!tiffGet(dir, 37385)) return "";
        std::string sb;
        if (iv & 1) sb += "Flash fired"; else sb += "Flash did not fire";
        if (iv & 4) {
            if (iv & 2) sb += ", return detected"; else sb += ", return not detected";
        }
        if (iv & 16) sb += ", auto";
        if (iv & 64) sb += ", red-eye reduction";
        return sb;
    }
    case 37386: {
        long long num, den;
        if (!tiffGetRational(dir, 37386, num, den)) return "";
        return javaDecimalFormat(1, 3, (double)num / (double)den) + " mm";
    }
    case 37510: {
        std::vector<unsigned char> b = tiffGetByteArray(dir, 37510);
        if (b.empty()) return "";
        // 10-byte charset prefix (ASCII/UNICODE/JIS), otherwise file.encoding.
        std::string raw((const char*)b.data(), b.size());
        if (b.size() >= 10) {
            std::string prefix = raw.substr(0, 10);
            if (prefix.rfind("ASCII", 0) == 0) {
                std::string body = raw.substr(10);
                size_t e = body.find_last_not_of(' ');
                if (e == std::string::npos) return "";
                return body.substr(0, e + 1);
            }
            if (prefix.rfind("UNICODE", 0) == 0) {
                std::string body = raw.substr(10);
                // UTF-16LE decode, trim trailing NUL/space.
                std::string out;
                for (size_t i = 0; i + 1 < body.size(); i += 2) {
                    wchar_t wc = (wchar_t)((unsigned char)body[i] | ((unsigned char)body[i + 1] << 8));
                    if (wc < 0x80) out += (char)wc;
                    else out += '?';
                }
                while (!out.empty() && (out.back() == '\0' || out.back() == ' ')) out.pop_back();
                return out;
            }
            if (prefix.rfind("JIS", 0) == 0) {
                std::string body = raw.substr(10);
                while (!body.empty() && body.back() == ' ') body.pop_back();
                return body;
            }
        }
        // Treat as ASCII (file.encoding on the test host).
        std::string body = raw;
        while (!body.empty() && body.back() == ' ') body.pop_back();
        return body;
    }
    case 40091: case 40092: case 40093: case 40094: case 40095: {
        std::vector<unsigned char> b = tiffGetByteArray(dir, tag);
        if (b.empty()) return "";
        std::string out;
        for (size_t i = 0; i + 1 < b.size(); i += 2) {
            wchar_t wc = (wchar_t)((unsigned char)b[i] | ((unsigned char)b[i + 1] << 8));
            if (wc < 0x80) out += (char)wc;
            else out += '?';
        }
        size_t b0 = out.find_first_not_of(" \t\r\n");
        size_t e0 = out.find_last_not_of(" \t\r\n");
        out = (b0 == std::string::npos) ? "" : out.substr(b0, e0 - b0 + 1);
        return out;
    }
    case 40960: return versionBytesDescription(dir, 40960, 2);
    case 40961: {
        long long iv = tiffGetInteger(dir, 40961);
        if (!tiffGet(dir, 40961)) return "";
        if (iv == 1) return "sRGB";
        return iv == 65535 ? "Undefined" : "Unknown (" + std::to_string(iv) + ")";
    }
    case 40962: { if (!tiffGet(dir, 40962)) return ""; return std::to_string(tiffGetInteger(dir, 40962)) + " pixels"; }
    case 40963: { if (!tiffGet(dir, 40963)) return ""; return std::to_string(tiffGetInteger(dir, 40963)) + " pixels"; }
    case 41486: case 41487: {
        long long num, den;
        if (!tiffGetRational(dir, tag, num, den)) return "";
        std::string res = indexedDescription(dir, 41488, 1, {"(No unit)", "Inches", "cm"});
        std::string unit = res.empty() ? "" : " " + vToLower(res);
        return rationalToSimpleString(den, num, true) + unit;
    }
    case 41488: return indexedDescription(dir, 41488, 1, {"(No unit)", "Inches", "cm"});
    case 41495: return indexedDescription(dir, 41495, 1, {"(Not defined)", "One-chip color area sensor", "Two-chip color area sensor", "Three-chip color area sensor", "Color sequential area sensor", "", "Trilinear sensor", "Color sequential linear sensor"});
    case 41728: return indexedDescription(dir, 41728, 1, {"Film Scanner", "Reflection Print Scanner", "Digital Still Camera (DSC)"});
    case 41729: return indexedDescription(dir, 41729, 1, {"Directly photographed image"});
    case 41985: return indexedDescription(dir, 41985, 0, {"Normal process", "Custom process"});
    case 41986: return indexedDescription(dir, 41986, 0, {"Auto exposure", "Manual exposure", "Auto bracket"});
    case 41987: return indexedDescription(dir, 41987, 0, {"Auto white balance", "Manual white balance"});
    case 41988: {
        long long num, den;
        if (!tiffGetRational(dir, 41988, num, den)) return "";
        return num == 0 ? "Digital zoom not used." : javaDecimalFormat(0, 1, (double)num / (double)den);
    }
    case 41989: {
        if (!tiffGet(dir, 41989)) return "";
        long long iv = tiffGetInteger(dir, 41989);
        return iv == 0 ? "Unknown" : std::to_string(iv) + "mm";
    }
    case 41990: return indexedDescription(dir, 41990, 0, {"Standard", "Landscape", "Portrait", "Night scene"});
    case 41991: return indexedDescription(dir, 41991, 0, {"None", "Low gain up", "Low gain down", "High gain up", "High gain down"});
    case 41992: return indexedDescription(dir, 41992, 0, {"None", "Soft", "Hard"});
    case 41993: return indexedDescription(dir, 41993, 0, {"None", "Low saturation", "High saturation"});
    case 41994: return indexedDescription(dir, 41994, 0, {"None", "Low", "Hard"});
    case 41996: return indexedDescription(dir, 41996, 0, {"Unknown", "Macro", "Close view", "Distant view"});
    default:
        return tagDescriptorFallback(dir, tag, v);
    }
}

std::string gpsDescription(const TiffDir &dir, int tag)
{
    const TiffValue *v = tiffGet(dir, tag);
    if (!v) return "";
    switch (tag) {
    case 0: return versionBytesDescription(dir, 0, 1);
    case 2: case 4: {
        std::vector<long long> num, den;
        int refTag = (tag == 2) ? 1 : 3;
        std::string ref = tiffGetString(dir, refTag);
        if (!tiffGetRationalArray(dir, tag, num, den) || num.size() != 3 || ref.empty())
            return tiffGetString(dir, tag);
        double d = std::fabs((double)num[0] / (double)den[0]) +
                   ((double)num[1] / (double)den[1]) / 60.0 +
                   ((double)num[2] / (double)den[2]) / 3600.0;
        if (ref == "S" || ref == "s" || ref == "W" || ref == "w") d = -d;
        int deg = (int)d;
        double dAbs = std::fabs((d - (double)deg) * 60.0);
        int minutes = (int)dAbs;
        double seconds = (dAbs - (double)minutes) * 60.0;
        return javaDecimalFormat(0, 2, deg) + "\xC2\xB0 " +
               javaDecimalFormat(0, 2, minutes) + "' " +
               javaDecimalFormat(0, 2, seconds) + "\"";
    }
    case 5: {
        long long iv = tiffGetInteger(dir, 5);
        if (tiffGet(dir, 5) == nullptr) return "";
        return iv == 0 ? "Sea level" : (iv == 1 ? "Below sea level" : "Unknown (" + std::to_string(iv) + ")");
    }
    case 6: {
        long long num, den;
        if (!tiffGetRational(dir, 6, num, den)) return "";
        return std::to_string(rationalIntValue(num, den)) + " metres";
    }
    case 7: {
        std::vector<long long> num, den;
        if (!tiffGetRationalArray(dir, 7, num, den) || num.size() < 3) return tiffGetString(dir, 7);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%02lld:%02lld:%s UTC",
                      (long long)rationalIntValue(num[0], den[0]),
                      (long long)rationalIntValue(num[1], den[1]),
                      decimalFormat00_00((double)num[2] / (double)den[2]).c_str());
        return buf;
    }
    case 9: {
        std::string s = tiffGetString(dir, 9);
        if (s.empty()) return "";
        std::string t = s;
        size_t b = t.find_first_not_of(" \t");
        size_t e = t.find_last_not_of(" \t");
        t = (b == std::string::npos) ? "" : t.substr(b, e - b + 1);
        if (t == "A" || t == "a") return "Active (Measurement in progress)";
        if (t == "V" || t == "v") return "Void (Measurement Interoperability)";
        return "Unknown (" + t + ")";
    }
    case 10: {
        std::string s = tiffGetString(dir, 10);
        if (s.empty()) return "";
        std::string t = s;
        size_t b = t.find_first_not_of(" \t");
        size_t e = t.find_last_not_of(" \t");
        t = (b == std::string::npos) ? "" : t.substr(b, e - b + 1);
        if (t == "2" || t == "2") return "2-dimensional measurement";
        if (t == "3" || t == "3") return "3-dimensional measurement";
        return "Unknown (" + t + ")";
    }
    case 12: {
        std::string s = tiffGetString(dir, 12);
        if (s.empty()) return "";
        std::string t = s;
        size_t b = t.find_first_not_of(" \t");
        size_t e = t.find_last_not_of(" \t");
        t = (b == std::string::npos) ? "" : t.substr(b, e - b + 1);
        if (t == "K" || t == "k") return "kph";
        if (t == "M" || t == "m") return "mph";
        if (t == "N" || t == "n") return "knots";
        return "Unknown (" + t + ")";
    }
    case 14: case 16: case 23: {
        std::string s = tiffGetString(dir, tag);
        if (s.empty()) return "";
        std::string t = s;
        size_t b = t.find_first_not_of(" \t");
        size_t e = t.find_last_not_of(" \t");
        t = (b == std::string::npos) ? "" : t.substr(b, e - b + 1);
        if (t == "T" || t == "t") return "True direction";
        if (t == "M" || t == "m") return "Magnetic direction";
        return "Unknown (" + t + ")";
    }
    case 15: case 17: case 24: {
        std::string s;
        long long num, den;
        if (tiffGetRational(dir, tag, num, den)) {
            s = javaDecimalFormat(0, 2, (double)num / (double)den);
        } else {
            s = tiffGetString(dir, tag);
        }
        size_t b = s.find_first_not_of(" \t");
        size_t e = s.find_last_not_of(" \t");
        if (b == std::string::npos || e < b) return "";
        s = s.substr(b, e - b + 1);
        return s + " degrees";
    }
    case 25: {
        std::string s = tiffGetString(dir, 25);
        if (s.empty()) return "";
        std::string t = s;
        size_t b = t.find_first_not_of(" \t");
        size_t e = t.find_last_not_of(" \t");
        t = (b == std::string::npos) ? "" : t.substr(b, e - b + 1);
        if (t == "K" || t == "k") return "kilometers";
        if (t == "M" || t == "m") return "miles";
        if (t == "N" || t == "n") return "knots";
        return "Unknown (" + t + ")";
    }
    case 30: {
        long long iv = tiffGetInteger(dir, 30);
        if (tiffGet(dir, 30) == nullptr) return "";
        return iv == 0 ? "No Correction" : (iv == 1 ? "Differential Corrected" : "Unknown (" + std::to_string(iv) + ")");
    }
    default:
        return tagDescriptorFallback(dir, tag, v);
    }
}

const std::map<int, std::string> &iptcTagNames()
{
    static const std::map<int, std::string> m = {
        {256, "Enveloped Record Version"}, {261, "Destination"}, {276, "File Format"},
        {278, "File Version"}, {286, "Service Identifier"}, {296, "Envelope Number"},
        {306, "Product Identifier"}, {316, "Envelope Priority"}, {326, "Date Sent"},
        {336, "Time Sent"}, {346, "Coded Character Set"}, {356, "Unique Object Name"},
        {376, "ARM Identifier"}, {378, "ARM Version"},
        {512, "Application Record Version"}, {515, "Object Type Reference"},
        {516, "Object Attribute Reference"}, {517, "Object Name"}, {519, "Edit Status"},
        {520, "Editorial Update"}, {522, "Urgency"}, {524, "Subject Reference"},
        {527, "Category"}, {532, "Supplemental Category(s)"}, {534, "Fixture Identifier"},
        {537, "Keywords"}, {538, "Content Location Code"}, {539, "Content Location Name"},
        {542, "Release Date"}, {547, "Release Time"}, {549, "Expiration Date"},
        {550, "Expiration Time"}, {552, "Special Instructions"}, {554, "Action Advised"},
        {557, "Reference Service"}, {559, "Reference Date"}, {562, "Reference Number"},
        {567, "Date Created"}, {572, "Time Created"}, {574, "Digital Date Created"},
        {575, "Digital Time Created"}, {577, "Originating Program"}, {582, "Program Version"},
        {587, "Object Cycle"}, {592, "By-line"}, {597, "By-line Title"}, {602, "City"},
        {604, "Sub-location"}, {607, "Province/State"},
        {612, "Country/Primary Location Code"}, {613, "Country/Primary Location Name"},
        {615, "Original Transmission Reference"}, {617, "Headline"}, {622, "Credit"},
        {627, "Source"}, {628, "Copyright Notice"}, {630, "Contact"},
        {632, "Caption/Abstract"}, {633, "Local Caption"}, {634, "Caption Writer/Editor"},
        {637, "Rasterized Caption"}, {642, "Image Type"}, {643, "Image Orientation"},
        {647, "Language Identifier"}, {662, "Audio Type"}, {663, "Audio Sampling Rate"},
        {664, "Audio Sampling Resolution"}, {665, "Audio Duration"}, {666, "Audio Outcue"},
        {696, "Job Identifier"}, {697, "Master Document Identifier"},
        {698, "Short Document Identifier"}, {699, "Unique Document Identifier"},
        {700, "Owner Identifier"}, {712, "Object Data Preview File Format"},
        {713, "Object Data Preview File Format Version"}, {714, "Object Data Preview Data"},
    };
    return m;
}

std::vector<IptcEntry> parseIptc(const std::vector<std::byte> &payload)
{
    std::vector<IptcEntry> out;
    const uint8_t *p = (const uint8_t*)payload.data();
    size_t n = payload.size();
    size_t i = 0;
    auto existing = [&](int tag) -> IptcEntry* {
        for (auto &e : out) if (e.tag == tag) return &e;
        return nullptr;
    };
    while (i < n) {
        if (p[i] != 28) { i++; continue; } // invalid marker, skip
        if (i + 5 >= n) return out;
        int record = p[i + 1];
        int dataset = p[i + 2];
        size_t len = ((size_t)p[i + 3] << 8) | p[i + 4];
        size_t dataStart = i + 5;
        if (dataStart + len > n) return out;
        int tag = dataset | (record << 8);
        std::vector<std::byte> data(payload.begin() + dataStart, payload.begin() + dataStart + len);
        if (len == 0) {
            IptcEntry e;
            e.tag = tag;
            out.push_back(e);
            i = dataStart + len;
            continue;
        }
        const uint8_t *d = (const uint8_t*)data.data();
        bool handled = false;
        switch (tag) {
        case 256: case 278: case 378: case 512: case 582: {
            if (len >= 2) {
                IptcEntry e;
                e.tag = tag;
                e.isInt = true;
                e.i = ((size_t)d[0] << 8) | d[1];
                out.push_back(e);
                handled = true;
            }
            break;
        }
        case 522: { // Urgency (single byte int)
            IptcEntry e;
            e.tag = tag;
            e.isInt = true;
            e.i = d[0];
            out.push_back(e);
            handled = true;
            break;
        }
        default:
            break;
        }
        if (!handled) {
            // String value; charset: assume ISO-8859-1 on this host.
            std::string s;
            for (size_t k = 0; k < len; k++) s += (char)d[k];
            IptcEntry *ex = existing(tag);
            if (ex) {
                ex->strArray.push_back(s);
            } else {
                IptcEntry e;
                e.tag = tag;
                e.str = s;
                out.push_back(e);
            }
        }
        i = dataStart + len;
    }
    return out;
}

std::string iptcDescription(const IptcEntry &e)
{
    if (e.isInt) return std::to_string(e.i);
    if (e.tag == 537) { // Keywords joined with ";"
        std::string s;
        if (!e.strArray.empty()) {
            for (size_t i = 0; i < e.strArray.size(); i++) {
                if (i) s += ";";
                s += e.strArray[i];
            }
        } else {
            s = e.str;
        }
        return s;
    }
    if (e.tag == 572 || e.tag == 575) { // Time Created / Digital Time Created
        std::string s = e.str;
        if (s.size() == 6 || s.size() == 11)
            return s.substr(0, 2) + ":" + s.substr(2, 2) + ":" + s.substr(4);
        return s;
    }
    return e.str;
}

std::vector<std::byte> imageMetaBytes(ImageData *img)
{
    if (img->source.empty()) {
        throw exception(string("Application"),
                        string("The ImageMetadata feature is not supported for BLOBS or Base64 strings."),
                        string("The ImageMetadata feature is not supported for BLOBS or Base64 strings."));
    }
    if (!img->sourceBytes.empty()) return img->sourceBytes;
    std::ifstream f(img->source, std::ios::binary);
    if (!f) {
        throw exception(string("Application"),
                        string("The ImageMetadata feature is not supported for BLOBS or Base64 strings."),
                        string("The ImageMetadata feature is not supported for BLOBS or Base64 strings."));
    }
    std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<std::byte> out;
    out.reserve(data.size());
    for (char c : data) out.push_back((std::byte)c);
    return out;
}

void buildExifStruct(ImageData *img, cfvariant &s)
{
    std::vector<std::byte> bytes = imageMetaBytes(img);
    std::vector<TiffDir> dirs;
    std::string fmt = vToLower(img->sourceFormat);
    if (fmt == "jpeg" || fmt == "jpg") {
        std::vector<std::vector<std::byte>> app1, app13;
        jpegAppSegments(bytes, app1, app13);
        for (auto &seg : app1) {
            if (seg.size() < 6) continue;
            if (memcmp(seg.data(), "Exif\0\0", 6) != 0) continue;
            parseTiff(seg, 6, dirs);
            break;
        }
    } else if (fmt == "tif" || fmt == "tiff") {
        parseTiff(bytes, 0, dirs);
    } else {
        // Other formats have no EXIF directory.
        return;
    }

    // The order the directories were added to com.drew's Metadata object is
    // 0=IFD0, 1=SubIFD, 2=GPS, 3=Interop, 4=Thumbnail. Only emit the kinds
    // that were actually created.
    for (int kind = 0; kind <= 4; kind++) {
        for (const auto &dir : dirs) {
            if (dir.kind != kind) continue;
            const auto &nameMap = (kind == 2) ? gpsTagNames() : exifTagNames();
            for (const auto &t : dir.tags) {
                std::string name = tagName(nameMap, t.tag);
                std::string desc = (kind == 2) ? gpsDescription(dir, t.tag)
                                               : exifDescription(dir, t.tag);
                s.structSet(string(name.c_str()), cfvariant(string(desc.c_str())));
            }
        }
    }
}

void buildIptcStruct(ImageData *img, cfvariant &s)
{
    std::vector<std::byte> bytes = imageMetaBytes(img);
    std::string fmt = vToLower(img->sourceFormat);
    if (fmt != "jpeg" && fmt != "jpg") return;
    std::vector<std::vector<std::byte>> app1, app13;
    jpegAppSegments(bytes, app1, app13);
    for (auto &seg : app13) {
        static const char preamble[] = "Photoshop 3.0";
        if (seg.size() < sizeof(preamble)) continue;
        if (memcmp(seg.data(), preamble, sizeof(preamble) - 1) != 0) continue;
        // Walk the IRB resource blocks.
        size_t pos = sizeof(preamble);
        const uint8_t *p = (const uint8_t*)seg.data();
        size_t n = seg.size();
        while (pos + 12 <= n) {
            uint16_t resId = (uint16_t)((p[pos + 4] << 8) | p[pos + 5]);
            size_t nameLen = p[pos + 6];
            if (pos + 7 + nameLen > n) break;
            size_t cur = pos + 7 + nameLen;
            if ((cur - pos) % 2 != 0) cur++;
            if (cur + 4 > n) break;
            uint32_t dataLen = ((uint32_t)p[cur] << 24) | ((uint32_t)p[cur + 1] << 16) |
                               ((uint32_t)p[cur + 2] << 8) | p[cur + 3];
            size_t dataStart = cur + 4;
            if (dataStart + dataLen > n) break;
            if (resId == 0x0404) {
                std::vector<std::byte> iptcBytes;
                iptcBytes.reserve(dataLen);
                for (size_t k = dataStart; k < dataStart + dataLen; k++)
                    iptcBytes.push_back((std::byte)p[k]);
                std::vector<IptcEntry> entries = parseIptc(iptcBytes);
                for (auto &e : entries) {
                    std::string name = tagName(iptcTagNames(), e.tag);
                    std::string desc = iptcDescription(e);
                    s.structSet(string(name.c_str()), cfvariant(string(desc.c_str())));
                }
                break;
            }
            size_t advance = dataStart + dataLen - pos;
            if (advance % 2 != 0) advance++;
            pos += advance;
        }
        break;
    }
}

} // namespace cfml

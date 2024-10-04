/**
 * @file tag_weboutput.cpp
 * @brief Web/HTTP/Output tag runtimes: <cfcookie>, <cfhtmlhead>,
 *        <cfsavecontent> and <cfsetting>.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <ctime>
#include <cstdio>
#include <cstring>
#include <string>

namespace cfml {

// ---- <cfsetting> ----

void cf_setting(const cfvariant *enablecfoutputonly, const cfvariant *showdebugoutput,
                const cfvariant *requesttimeout)
{
    (void)showdebugoutput;
    (void)requesttimeout;
    if (enablecfoutputonly) {
        cf_cfoutputonly_set(cfmlBoolean(enablecfoutputonly, false));
    }
}

// ---- <cfhtmlhead> ----

void cf_htmlhead_append(const cfvariant *text)
{
    if (!text) return;
    cfml::response().headContent.append(variantToString(*text));
}

// ---- <cfsavecontent> ----

namespace {

// CF's CFVariableLexer.isValidVariableName: the first char may be any Java
// identifier start/part char (letters, digits, '_', '$'), subsequent chars any
// Java identifier part char, and a '.' resets the segment (a dotted name is
// valid unless it ends with '.' or contains an empty segment). This is more
// permissive than the engine's cfmlIsValidVariableName (leading digits are
// allowed, like CF).
bool savecontentVarNameValid(const webstrada::string &name)
{
    if (name.isEmpty()) return false;
    bool first = true;
    bool inSegment = true;
    for (int i = 0; i < name.length(); i++) {
        char c = name.at(i);
        if (c == '.') {
            if (first || i == name.length() - 1) return false;
            inSegment = false;
            continue;
        }
        if (c == '_' || c == '$' || isalpha((unsigned char)c)) {
            inSegment = true;
        } else if (!first && isdigit((unsigned char)c)) {
            inSegment = true;
        } else {
            return false;
        }
        first = false;
    }
    (void)inSegment;
    return true;
}

} // namespace

void cf_savecontent_validate(const cfvariant *varName)
{
    if (!varName) return;
    string name = variantToString(*varName);
    if (!savecontentVarNameValid(name)) {
        throw webstrada::exception("Expression", "Cannot set variable with name " + name + ".", "");
    }
}

string *cf_savecontent_begin(string *realOut)
{
    (void)realOut;
    return silent_buf_push();
}

void cf_savecontent_end_assign(string *captured, void *cgi, void *server, void *cookie,
                               void *application, void *session, void *url, void *form,
                               void *variables, const cfvariant *varName)
{
    string body;
    if (captured) {
        const char *d = captured->constData();
        if (d) body.append(d, captured->length());
    }
    if (varName) {
        std::string name = safe_to_std_string(*varName);
        cfvariant val(body);
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         name.c_str(), &val);
    }
}

void cf_savecontent_end(string *captured, void *cgi, void *server, void *cookie,
                        void *application, void *session, void *url, void *form,
                        void *variables, const cfvariant *varName)
{
    cf_savecontent_end_assign(captured, cgi, server, cookie, application, session, url, form,
                              variables, varName);
    silent_buf_pop();
}

// ---- <cfcookie> helpers ----

namespace {

// CF's URLEncoder.encode: only ASCII letters/digits are left as-is, every other
// byte (UTF-8) is percent-encoded with uppercase hex (space -> %20, never '+').
webstrada::string cookieUrlEncode(const webstrada::string &value)
{
    std::vector<std::byte> bytes;
    stringToBytes(value, "UTF-8", bytes);
    webstrada::string out;
    for (auto b : bytes) {
        unsigned char c = static_cast<unsigned char>(b);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            out.append(static_cast<char>(c));
        } else {
            static const char *hex = "0123456789ABCDEF";
            out.append('%');
            out.append(hex[c >> 4]);
            out.append(hex[c & 0xF]);
        }
    }
    return out;
}

// CF's CookieTag.validateCookieName: non-empty, no space/comma/semicolon, not
// starting with '$'.
bool cookieNameValid(const webstrada::string &name)
{
    if (name.isEmpty()) return false;
    for (int i = 0; i < name.length(); i++) {
        char c = name.at(i);
        if (c == ' ' || c == ',' || c == ';') return false;
    }
    return name.at(0) != '$';
}

// A value is written bare (no quoting) iff every char is a printable ASCII
// token char; the separator set is what CF's Tomcat cookie processor quotes.
bool cookieValueIsToken(const webstrada::string &value)
{
    for (int i = 0; i < value.length(); i++) {
        unsigned char c = static_cast<unsigned char>(value.at(i));
        if (c < 0x21 || c > 0x7E) return false;
        if (c == '"' || c == '(' || c == ')' || c == ',' || c == ':' || c == ';' ||
            c == '<' || c == '=' || c == '>' || c == '?' || c == '@' || c == '[' ||
            c == '\\' || c == ']' || c == '{' || c == '}') {
            return false;
        }
    }
    return true;
}

webstrada::string quoteCookieValue(const webstrada::string &value)
{
    webstrada::string out;
    out.append('"');
    for (int i = 0; i < value.length(); i++) {
        char c = value.at(i);
        if (c == '"' || c == '\\') out.append('\\');
        out.append(c);
    }
    out.append('"');
    return out;
}

// RFC 1123 "EEE, dd MMM yyyy HH:mm:ss GMT" (24-hour clock) with the day/month
// names CF's Tomcat emits.
void formatRfc1123(time_t t, char *buf, size_t buflen)
{
    struct tm tmVal;
    gmtime_r(&t, &tmVal);
    static const char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    snprintf(buf, buflen, "%s, %02d %s %04d %02d:%02d:%02d GMT",
             wday[tmVal.tm_wday], tmVal.tm_mday, mon[tmVal.tm_mon], tmVal.tm_year + 1900,
             tmVal.tm_hour, tmVal.tm_min, tmVal.tm_sec);
}

// CF's CookieScope.getExpiresHeaderForMaxAge: "EEE, dd-MMM-yyyy hh:mm:ss zzz"
// with a 12-hour clock (no AM/PM marker) in GMT; epoch for maxAge 0.
void formatRfc850(int maxAge, char *buf, size_t buflen)
{
    time_t now = time(nullptr);
    time_t expiry = (maxAge == 0) ? 0 : now + maxAge;
    struct tm tmVal;
    gmtime_r(&expiry, &tmVal);
    static const char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int h12 = tmVal.tm_hour % 12;
    if (h12 == 0) h12 = 12;
    snprintf(buf, buflen, "%s, %02d-%s-%04d %02d:%02d:%02d GMT",
             wday[tmVal.tm_wday], tmVal.tm_mday, mon[tmVal.tm_mon], tmVal.tm_year + 1900,
             h12, tmVal.tm_min, tmVal.tm_sec);
}

// CF's CookieTag.isDateFormat regex: \d\d?/\d\d?/\d\d(\d\d)? (full match).
bool cookieIsSlashDate(const webstrada::string &s)
{
    size_t i = 0;
    auto digits = [&](int n) {
        for (int k = 0; k < n; k++) {
            if (i >= (size_t)s.length() || !isdigit((unsigned char)s.at((int)i))) return false;
            i++;
        }
        return true;
    };
    if (!digits(1)) return false;
    if (i < (size_t)s.length() && isdigit((unsigned char)s.at((int)i))) i++;
    if (i >= (size_t)s.length() || s.at((int)i) != '/') return false;
    i++;
    if (!digits(1)) return false;
    if (i < (size_t)s.length() && isdigit((unsigned char)s.at((int)i))) i++;
    if (i >= (size_t)s.length() || s.at((int)i) != '/') return false;
    i++;
    if (!digits(2)) return false;
    if (i < (size_t)s.length()) {
        if (!digits(2)) return false;
    }
    return i == (size_t)s.length();
}

// The number of whole days since the engine's epoch (1899-12-31) for a
// CFML DateTime value; used for the max-age diff.
double cookieNowDays()
{
    struct tm nowTm;
    time_t t = time(nullptr);
    localtime_r(&t, &nowTm);
    return tmToDays(nowTm);
}

// CF's Cast._double for the processDouble path: strict whole-string number
// first, then a date-string parse. Returns true and sets `d` on success.
bool cookieStringToDouble(const webstrada::string &s, double &d)
{
    std::string str = safe_to_std_string(s);
    size_t b = 0, e = str.size();
    while (b < e && std::isspace((unsigned char)str[b])) b++;
    while (e > b && std::isspace((unsigned char)str[e - 1])) e--;
    if (b == e) return false;
    std::string trimmed = str.substr(b, e - b);
    char *end = nullptr;
    double v = strtod(trimmed.c_str(), &end);
    if (end == trimmed.c_str()) return false;
    while (*end && std::isspace((unsigned char)*end)) end++;
    if (*end != '\0') {
        double days = 0.0;
        if (parseDateTimeStr(s, days)) {
            d = days;
            return true;
        }
        return false;
    }
    d = v;
    return true;
}

// Java's (int) narrowing cast saturates out-of-range doubles; CF relies on
// that for max-age values computed from dates far in the future.
int saturatingInt(double d)
{
    if (d >= 2147483647.0) return 2147483647;
    if (d <= -2147483648.0) return -2147483648;
    return static_cast<int>(d);
}

// CF's processDouble: maxAge = (int)(86400.0 * value) with Java's saturating
// narrowing cast, then negative clamps to 0.
int cookieProcessDouble(double v)
{
    int maxage = saturatingInt(86400.0 * v);
    return maxage < 0 ? 0 : maxage;
}

// CF's CookieTag.setExpires / CookieScope.setExpires. Returns the max-age in
// seconds (-1 for a session cookie).
int cookieExpiresToMaxAge(const cfvariant *expires)
{
    if (!expires) return -1;

    if (expires->m_type == cfvariant::DateTime) {
        double diff = (expires->m_double - cookieNowDays()) * 86400.0;
        return diff < 0 ? 0 : saturatingInt(diff);
    }
    if (expires->m_type == cfvariant::Number || expires->m_type == cfvariant::Long ||
        expires->m_type == cfvariant::Float) {
        double d = (expires->m_type == cfvariant::Number)
                       ? static_cast<double>(expires->m_int)
                       : (expires->m_type == cfvariant::Long) ? static_cast<double>(expires->m_long)
                                                              : expires->m_double;
        return cookieProcessDouble(d);
    }

    string s = variantToString(*expires);
    if (s.compareCaseInsensitive("now") == 0) return 0;
    if (s.compareCaseInsensitive("never") == 0) return 946080000;
    if (s.compareCaseInsensitive("session") == 0) return -1;

    if (cookieIsSlashDate(s)) {
        // M/D/YY or M/D/YYYY: normalize a 2-digit year with Java's
        // SimpleDateFormat rule (00-69 -> 2000s, 70-99 -> 1900s) then diff.
        std::string str = safe_to_std_string(s);
        size_t p1 = str.find('/');
        size_t p2 = str.find('/', p1 + 1);
        std::string year = str.substr(p2 + 1);
        if (year.size() == 2) {
            int yy = atoi(year.c_str());
            int yyyy = (yy < 70) ? 2000 + yy : 1900 + yy;
            str = str.substr(0, p2 + 1) + std::to_string(yyyy);
        }
        double days = 0.0;
        if (parseDateTimeStr(str.c_str(), days)) {
            double diff = (days - cookieNowDays()) * 86400.0;
            return diff < 0 ? 0 : saturatingInt(diff);
        }
    }

    double d = 0.0;
    if (!cookieStringToDouble(s, d)) {
        throw webstrada::exception(("The value " + safe_to_std_string(s.trimmed()) +
                                   " cannot be converted to a number.").c_str());
    }
    return cookieProcessDouble(d);
}

} // namespace

void cf_cookie_tag(cfvariant *cookie, const cfvariant *name, const cfvariant *value,
                   const cfvariant *expires, const cfvariant *secure, const cfvariant *path,
                   const cfvariant *domain, const cfvariant *httponly,
                   const cfvariant *encodevalue, const cfvariant *preservecase,
                   const cfvariant *samesite)
{
    if (!name) {
        throw webstrada::exception("cfcookie", "Missing NAME attribute");
    }

    string nameStr = variantToString(*name);
    string samesiteStr = samesite ? variantToString(*samesite) : string();
    bool preserveCase = preservecase ? cfmlBoolean(preservecase, false) : false;
    if (!preserveCase) {
        nameStr.toUpper();
    }
    if (!cookieNameValid(nameStr)) {
        throw webstrada::exception("Application", "An error has occurred while trying to create a cookie with name " + nameStr + ".", "");
    }

    string valueStr = value ? stripCRLF(variantToString(*value)) : string();
    bool encode = (encodevalue == nullptr) || cfmlBoolean(encodevalue, true);
    string cookieValue = valueStr;
    if (encode) {
        cookieValue = cookieUrlEncode(valueStr);
    }

    int maxAge = cookieExpiresToMaxAge(expires);
    bool isSecure = secure ? cfmlBoolean(secure, false) : false;
    bool isHttpOnly = httponly ? cfmlBoolean(httponly, false) : false;
    string domainStr = domain ? stripCRLF(variantToString(*domain)) : string();
    string pathStr = path ? stripCRLF(variantToString(*path)) : string();
    if (pathStr.isEmpty()) pathStr = "/";

    // The raw (unencoded) value lands in the current request's COOKIE scope
    // under the (possibly uppercased) cookie name, like CF.
    if (cookie && cookie->m_type == cfvariant::Struct) {
        cookie->structSet(nameStr, cfvariant(valueStr));
    }

    // The Set-Cookie header body. Plain non-empty cookies without SameSite are
    // serialized by CF's Tomcat (quoted value -> Version=1/Max-Age, RFC1123
    // dates); empty or SameSite cookies go through CF's own createCookieHeader
    // (raw value, RFC850 dates). Both orders match the RDS-host output.
    webstrada::string body;
    bool samesiteSet = !samesiteStr.isEmpty();
    bool valueEmpty = valueStr.isEmpty();

    if (samesiteSet || valueEmpty) {
        body = nameStr;
        body += "=";
        if (samesiteSet) {
            body += cookieValue;
        } else {
            body += "\"\"";
        }
        if (!domainStr.isEmpty()) {
            body += "; Domain=";
            body += domainStr;
        }
        if (maxAge >= 0) {
            char date[64];
            formatRfc850(maxAge, date, sizeof(date));
            body += "; Max-Age=";
            body += string::number(maxAge);
            body += "; Expires=";
            body += date;
        }
        body += "; Path=";
        body += pathStr;
        if (isSecure) body += "; Secure";
        if (isHttpOnly) body += "; HttpOnly";
        if (samesiteSet) {
            body += "; SameSite=";
            body += samesiteStr;
        }
    } else {
        bool quoted = !cookieValueIsToken(cookieValue);
        body = nameStr;
        body += "=";
        if (quoted) {
            body += quoteCookieValue(cookieValue);
            body += "; Version=1";
            if (maxAge >= 0) {
                body += "; Max-Age=";
                body += string::number(maxAge);
            }
        } else {
            body += cookieValue;
        }
        if (!domainStr.isEmpty()) {
            body += "; Domain=";
            body += domainStr;
        }
        if (maxAge >= 0) {
            time_t now = time(nullptr);
            time_t expiry = (maxAge == 0) ? 10 : now + maxAge;
            char date[64];
            formatRfc1123(expiry, date, sizeof(date));
            body += "; Expires=";
            body += date;
        }
        body += "; Path=";
        body += pathStr;
        if (isSecure) body += "; Secure";
        if (isHttpOnly) body += "; HttpOnly";
    }

    auto &r = response();
    if (r.committed) return;
    r.cookies.push_back(std::string(body.constData(), body.length()));
}

} // namespace cfml

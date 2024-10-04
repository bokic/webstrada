/**
 * @file tag_login.cpp
 * @brief <cflogin> / <cfloginuser> / <cflogout> runtime + the auth functions.
 *
 * Mirrors ColdFusion 2025's AuthenticateTag / UserTag / LogoutTag backed by
 * SecurityScopeTracker + SecurityManager:
 *
 * - An auth token is base64("username\r appToken\r timeMillis\r nonceHex")
 *   where nonceHex is the uppercase hex of 8 random bytes (CF's
 *   createAuthToken). It is carried by the CFAUTHORIZATION_<appToken> cookie
 *   (default) or the session scope (loginstorage="session").
 * - A server-wide pool (SQLite cf_security table, CF's msecurityPool) maps the
 *   token to a SecurityTable {username, password, roles, appToken, idle
 *   timeout, last access}. The token itself is opaque to the browser; only the
 *   pool resolves it back to a login.
 * - <cflogin> runs its body when the request has no live login; when it does,
 *   the body is skipped (doStartTag SKIP_BODY). Inside the body a
 *   <cfloginuser> binds name/password/roles to the enclosing <cflogin>; at the
 *   end tag the login is committed (token created, pool entry stored, cookie /
 *   session key set) — the request is only "logged in" after </cflogin>,
 *   matching CF (verified: IsUserLoggedIn() is NO inside the body even after
 *   <cfloginuser>). If the body runs but no <cfloginuser> did, the end tag
 *   logs out (clears the cookie).
 * - When <cfloginuser> is used OUTSIDE a <cflogin>, the request is logged in
 *   directly with no cookie (CF's UserTag without an AuthenticateTag ancestor
 *   calls fContext.setSecureTable directly — verified: no Set-Cookie).
 * - j_username/j_password posted (URL or FORM scope) populate a `cflogin`
 *   struct {NAME, PASSWORD} in the variables scope during the body (CF's
 *   loginStruct); it is removed at the end tag.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/scope_store.h>
#include <webstrada/string.h>

#include <openssl/rand.h>

#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace cfml {

using webstrada::cfvariant;
using webstrada::string;

// ---- Per-request security state (CF's FusionContext secure table) ----

static thread_local SecurityContext g_sec;

void security_reset()
{
    g_sec = SecurityContext{};
}

const SecurityContext &security_context()
{
    return g_sec;
}

static cfvariant *boolVariant(bool val)
{
    auto *v = new cfvariant(cfvariant::Boolean);
    v->m_bool = val;
    return v;
}

// ---- Server-wide auth pool helpers (CF's SecurityScopeTracker) ----

static ScopeStore *securityStore()
{
    return scope_context().store;
}

static int64_t nowMillis()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

// Uppercase hex of `n` random bytes (CF's MD5.stringify(8 random bytes)).
static std::string randomNonce()
{
    unsigned char buf[8];
    if (RAND_bytes(buf, sizeof(buf)) != 1) {
        throw webstrada::exception("cflogin", "Secure random number generation failed.", "");
    }
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char b : buf) {
        out += hex[b >> 4];
        out += hex[b & 0x0F];
    }
    return out;
}

static std::string base64Encode(const std::string &in)
{
    static const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    size_t n = in.size();
    while (i + 2 < n) {
        unsigned int v = (static_cast<unsigned int>(static_cast<unsigned char>(in[i])) << 16)
            | (static_cast<unsigned int>(static_cast<unsigned char>(in[i+1])) << 8)
            | static_cast<unsigned int>(static_cast<unsigned char>(in[i+2]));
        out += b64[(v >> 18) & 0x3F];
        out += b64[(v >> 12) & 0x3F];
        out += b64[(v >> 6) & 0x3F];
        out += b64[v & 0x3F];
        i += 3;
    }
    if (i < n) {
        unsigned int v = static_cast<unsigned int>(static_cast<unsigned char>(in[i])) << 16;
        if (i + 1 < n) v |= static_cast<unsigned int>(static_cast<unsigned char>(in[i+1])) << 8;
        out += b64[(v >> 18) & 0x3F];
        out += b64[(v >> 12) & 0x3F];
        if (i + 1 < n) {
            out += b64[(v >> 6) & 0x3F];
            out += '=';
        } else {
            out += "==";
        }
    }
    return out;
}

static std::string base64Decode(const std::string &in)
{
    auto valOf = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    std::string out;
    size_t i = 0;
    while (i < in.size()) {
        if (in[i] == '=' || in[i] == '\r' || in[i] == '\n') { i++; continue; }
        int c0 = valOf(in[i]);
        if (c0 < 0 || i + 1 >= in.size()) break;
        int c1 = valOf(in[i + 1]);
        if (c1 < 0) break;
        unsigned int acc = (c0 << 18) | (c1 << 12);
        out += static_cast<char>((acc >> 16) & 0xFF);
        i += 2;
        if (i < in.size() && in[i] != '=') {
            int c2 = valOf(in[i]);
            if (c2 < 0) break;
            acc |= (c2 << 6);
            out += static_cast<char>((acc >> 8) & 0xFF);
            i += 1;
            if (i < in.size() && in[i] != '=') {
                int c3 = valOf(in[i]);
                if (c3 < 0) break;
                acc |= c3;
                out += static_cast<char>(acc & 0xFF);
                i += 1;
            }
        }
    }
    return out;
}

// Build the auth token (CF's SecurityManager.createAuthToken).
static std::string createAuthToken(const std::string &username, const std::string &appToken,
                                   const std::string &password, int64_t idleMs)
{
    std::string plain = username + "\r" + appToken + "\r" + std::to_string(nowMillis()) + "\r" + randomNonce();
    std::string token = base64Encode(plain);
    if (ScopeStore *store = securityStore()) {
        // The auth cache keeps the nonce->password mapping for verification;
        // the cf_security table stores the full SecurityTable keyed by token.
        store->storeSecurity(token, username, password, "", appToken, idleMs, nowMillis());
    }
    return token;
}

// Resolve a token to its SecurityContext. Mirrors SecurityManager.verifyAuth
// (the token must be decodable and its nonce registered — here the pool row is
// the registration) + SecurityScopeTracker.getSecurity.
bool security_resolve_token(const string &token, SecurityContext *out)
{
    if (!out) return false;
    ScopeStore *store = securityStore();
    if (!store) return false;
    std::string t(token.constData(), token.length());
    std::string username, password, roles, appToken;
    int64_t maxInactiveMs = 0;
    if (!store->loadSecurity(t, nowMillis(), username, password, roles, appToken, maxInactiveMs)) {
        return false;
    }
    out->loggedIn = true;
    out->username = username.c_str();
    out->password = password.c_str();
    out->appToken = appToken.c_str();
    out->roles.clear();
    // roles are stored '\r'-joined (never contain commas in CF's model); split
    // on '\r'.
    std::string r;
    for (size_t i = 0; i <= roles.size(); i++) {
        if (i == roles.size() || roles[i] == '\r') {
            if (!r.empty()) out->roles.push_back(r.c_str());
            r.clear();
        } else {
            r += roles[i];
        }
    }
    return true;
}

void security_store_table(const string &token, const SecurityContext &ctx, int64_t maxInactiveMs)
{
    ScopeStore *store = securityStore();
    if (!store) return;
    std::string roles;
    for (size_t i = 0; i < ctx.roles.size(); i++) {
        if (i) roles += "\r";
        roles += std::string(ctx.roles[i].constData(), ctx.roles[i].length());
    }
    std::string t(token.constData(), token.length());
    store->storeSecurity(t, std::string(ctx.username.constData(), ctx.username.length()),
                         std::string(ctx.password.constData(), ctx.password.length()),
                         roles, std::string(ctx.appToken.constData(), ctx.appToken.length()),
                         maxInactiveMs, nowMillis());
}

void security_remove_token(const string &token)
{
    ScopeStore *store = securityStore();
    if (!store) return;
    std::string t(token.constData(), token.length());
    store->removeSecurity(t);
}

void security_remove_by_app_token(const string &appToken)
{
    ScopeStore *store = securityStore();
    if (!store) return;
    store->removeSecurityByAppToken(std::string(appToken.constData(), appToken.length()));
}

// ---- Cookie helpers (CF's SecurityScopeTracker.createSecurityCookie + the
// Tomcat / createCookieHeader serializations, matching tag_weboutput) ----

// The name of the CFAUTHORIZATION cookie for `appToken`.
static std::string authCookieName(const std::string &appToken)
{
    return "CFAUTHORIZATION_" + appToken;
}

// A value is written bare iff every char is a printable ASCII token char.
static bool cookieValueIsToken(const std::string &value)
{
    for (unsigned char c : value) {
        if (c < 0x21 || c > 0x7E) return false;
        if (c == '"' || c == '(' || c == ')' || c == ',' || c == ':' || c == ';' ||
            c == '<' || c == '=' || c == '>' || c == '?' || c == '@' || c == '[' ||
            c == '\\' || c == ']' || c == '{' || c == '}') {
            return false;
        }
    }
    return true;
}

static std::string quoteCookieValue(const std::string &value)
{
    std::string out;
    out += '"';
    for (char c : value) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    out += '"';
    return out;
}

// RFC 850 "EEE, dd-MMM-yyyy hh:mm:ss GMT" (12-hour clock, no AM/PM) for the
// CF-owned createCookieHeader path (empty values / SameSite cookies).
static std::string formatRfc850(int maxAge)
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
    char buf[64];
    snprintf(buf, sizeof(buf), "%s, %02d-%s-%04d %02d:%02d:%02d GMT",
             wday[tmVal.tm_wday], tmVal.tm_mday, mon[tmVal.tm_mon], tmVal.tm_year + 1900,
             h12, tmVal.tm_min, tmVal.tm_sec);
    return buf;
}

// Set the CFAUTHORIZATION cookie. `token` empty -> logout cookie (maxAge 0).
// `domain` empty -> no Domain attribute. Mirrors CF's login cookie bytes
// (verified on CF 2025):
//   login:  CFAUTHORIZATION_<tok>="<b64>"; Version=1; [Domain=..;] Path=/; HttpOnly
//           (or bare value when the base64 has no '=' padding)
//   logout: CFAUTHORIZATION_<tok>=""; Max-Age=0; Expires=<RFC850 epoch>; Path=/; HttpOnly
static void setAuthCookie(const std::string &appToken, const std::string &token,
                          const std::string &domain)
{
    auto &r = response();
    if (r.committed) return;
    std::string body;
    if (token.empty()) {
        // CF's own createCookieHeader path (empty value).
        body = authCookieName(appToken) + "=\"\"";
        body += "; Max-Age=0; Expires=";
        body += formatRfc850(0);
        if (!domain.empty()) { body += "; Domain="; body += domain; }
        body += "; Path=/";
        body += "; HttpOnly";
    } else {
        // Tomcat path: quoted + Version=1 when the value is not a token.
        if (cookieValueIsToken(token)) {
            body = authCookieName(appToken) + "=" + token;
        } else {
            body = authCookieName(appToken) + "=" + quoteCookieValue(token);
            body += "; Version=1";
        }
        if (!domain.empty()) { body += "; Domain="; body += domain; }
        body += "; Path=/";
        body += "; HttpOnly";
    }
    r.cookies.push_back(body);
}

// ---- Login frame stack (the enclosing <cflogin> that <cfloginuser> binds to;
// CF's findAncestorWithClass) ----

struct LoginFrame {
    bool bodyRan = false;             // the cflogin body executed
    bool userSet = false;             // a <cfloginuser> ran in the body
    string name;                      // cfloginuser name
    string password;                  // cfloginuser password
    std::vector<string> roles;        // cfloginuser roles (in order)
    string appToken;                  // application token of this cflogin
    string cookieDomain;
    int64_t idleTimeoutMs = 0;        // idletimeout in ms (0 = CF default 30 min)
};

static thread_local std::vector<LoginFrame> g_loginFrames;

// Read a named value from a struct scope (case-insensitive lookup, uppercased
// keys like the request scopes).
static bool scopeString(const cfvariant *scope, const char *key, string &out)
{
    if (!scope || scope->m_type != cfvariant::Struct) return false;
    auto it = scope->m_struct->find(string(key));
    if (it == scope->m_struct->end()) return false;
    out = const_cast<cfvariant&>(it->second).toString();
    return true;
}

// Resolve the application token (CF's AuthenticateTag.getApplicationToken):
// the applicationtoken attribute, else the current application name.
static std::string currentAppToken(const cfvariant *applicationtoken)
{
    if (applicationtoken) {
        string v = const_cast<cfvariant*>(applicationtoken)->toString();
        return std::string(v.constData(), v.length());
    }
    const std::string &appName = scope_context().appName;
    if (appName == "<unnamed>") return "";
    return appName;
}

// Resolve an existing login for `appToken` from the request's cookie or
// session scope. Returns the auth token (empty = none). When found and live it
// also fills g_sec (CF's doStartTag: getSecurity from the cookie/session key).
static string resolveExistingLogin(cfvariant *cookie, cfvariant *session,
                                   const std::string &appToken)
{
    string name = authCookieName(appToken).c_str();
    string token;
    bool storeInSession = scope_context().loginStorageIsSession;
    if (storeInSession) {
        if (!scopeString(session, name.constData(), token)) return string();
    } else {
        if (!scopeString(cookie, name.constData(), token)) return string();
    }
    if (token.isEmpty()) return string();
    SecurityContext ctx;
    if (security_resolve_token(token, &ctx)) {
        g_sec = ctx;
        return token;
    }
    return string();
}

// Remove the `cflogin` variable from the variables scope (CF's doEndTag /
// doFinally removeAttribute).
static void removeCfloginVar(cfvariant *variables)
{
    if (!variables || variables->m_type != cfvariant::Struct) return;
    variables->m_struct->erase(string("cflogin"));
    if (variables->m_structData && variables->m_structData->insertOrder.size()) {
        auto &io = variables->m_structData->insertOrder;
        for (size_t i = 0; i < io.size(); i++) {
            if (io[i].equals("cflogin")) { io.erase(io.begin() + (long)i); break; }
        }
    }
}

// Set the `cflogin` struct {NAME, PASSWORD} in the variables scope.
static void setCfloginVar(cfvariant *variables, const string &name, const string &password)
{
    if (!variables || variables->m_type != cfvariant::Struct) return;
    cfvariant loginStruct(cfvariant::Struct);
    loginStruct.structSet("name", cfvariant(name));
    loginStruct.structSet("password", cfvariant(password));
    variables->structSet("cflogin", loginStruct);
}

int cf_login_begin(string *out, void *cgi, void *server, void *cookie,
                   void *application, void *session, void *url, void *form,
                   void *variables,
                   const cfvariant *idletimeout, const cfvariant *usebasicauth,
                   const cfvariant *allowconcurrent, const cfvariant *applicationtoken,
                   const cfvariant *cookiedomain)
{
    (void)out; (void)cgi; (void)server; (void)application;
    (void)usebasicauth; (void)allowconcurrent;

    std::string appToken = currentAppToken(applicationtoken);
    LoginFrame frame;
    frame.appToken = appToken.c_str();
    if (cookiedomain) {
        string d = const_cast<cfvariant*>(cookiedomain)->toString();
        frame.cookieDomain = d.constData();
    }
    // idletimeout is in seconds (CF's docs); SecurityTable.setMaxInactiveInterval
    // multiplies by 1000 to store ms. CF's AuthenticateTag field default
    // `_maxInactiveInterval = 1800000` is passed through that *1000, so the
    // effective default idle timeout is 1800000 * 1000 ms (~20.8 days), NOT 30
    // minutes (verified in the decompiled 2025 sources). We reproduce it.
    frame.idleTimeoutMs = 1800000LL * 1000LL;
    if (idletimeout) {
        int64_t secs = static_cast<int64_t>(getIntValue(*const_cast<cfvariant*>(idletimeout)));
        frame.idleTimeoutMs = secs > 0 ? secs * 1000 : secs;
    }

    // An existing live login skips the body.
    string existing = resolveExistingLogin(static_cast<cfvariant*>(cookie),
                                           static_cast<cfvariant*>(session), appToken);
    if (!existing.isEmpty()) {
        // doStartTag returns SKIP_BODY when a security table was found.
        frame.bodyRan = false;
        g_loginFrames.push_back(frame);
        return 0;
    }

    // Look for j_username/j_password in the URL and FORM scopes (CF's
    // SCOPES_TO_SEARCH = {URL, FORM}).
    string juser, jpass;
    bool haveCreds = false;
    if (scopeString(static_cast<cfvariant*>(url), "J_USERNAME", juser)) {
        scopeString(static_cast<cfvariant*>(url), "J_PASSWORD", jpass);
        haveCreds = true;
    } else if (scopeString(static_cast<cfvariant*>(form), "J_USERNAME", juser)) {
        scopeString(static_cast<cfvariant*>(form), "J_PASSWORD", jpass);
        haveCreds = true;
    }

    frame.bodyRan = true;
    g_loginFrames.push_back(frame);

    if (haveCreds) {
        // CF's AuthenticateTag sets the `cflogin` struct {name, password} in
        // the local scope when auth info was found, then runs the body.
        setCfloginVar(static_cast<cfvariant*>(variables), juser, jpass);
    }
    return 1;
}

void cf_login_end(string *out, void *cgi, void *server, void *cookie,
                  void *application, void *session, void *url, void *form,
                  void *variables,
                  const cfvariant *idletimeout, const cfvariant *usebasicauth,
                  const cfvariant *allowconcurrent, const cfvariant *applicationtoken,
                  const cfvariant *cookiedomain, int runBody)
{
    (void)out; (void)cgi; (void)server; (void)application;
    (void)usebasicauth; (void)allowconcurrent;

    LoginFrame frame;
    if (!g_loginFrames.empty()) {
        frame = g_loginFrames.back();
        g_loginFrames.pop_back();
    }
    std::string appToken = frame.appToken.constData() ? std::string(frame.appToken.constData(), frame.appToken.length()) : currentAppToken(applicationtoken);
    std::string domain = frame.cookieDomain.constData() ? std::string(frame.cookieDomain.constData(), frame.cookieDomain.length()) : std::string();

    // If the body ran and a <cfloginuser> bound credentials, commit the login.
    // Otherwise (body ran with no loginuser, or the body was skipped because an
    // existing login was already active) this is a no-op / logout.
    if (frame.bodyRan && frame.userSet) {
        g_sec.loggedIn = true;
        g_sec.username = frame.name;
        g_sec.password = frame.password;
        g_sec.roles = frame.roles;
        g_sec.appToken = frame.appToken;

        int64_t idleMs = frame.idleTimeoutMs;
        if (idleMs <= 0) idleMs = 0;

        std::string token = createAuthToken(std::string(g_sec.username.constData(), g_sec.username.length()),
                                            appToken,
                                            std::string(g_sec.password.constData(), g_sec.password.length()),
                                            idleMs);
        // Store the roles with the token (createAuthToken stored an empty
        // roles field; re-store with the real roles).
        security_store_table(string(token.c_str()), g_sec, idleMs);

        if (scope_context().loginStorageIsSession) {
            // Store the auth key in the session scope (CF's
            // setupSecurityContext with storeSecurityKeyInSession).
            if (session && static_cast<cfvariant*>(session)->m_type == cfvariant::Struct) {
                static_cast<cfvariant*>(session)->structSet(authCookieName(appToken).c_str(), cfvariant(token.c_str()));
            }
        } else {
            setAuthCookie(appToken, token, domain);
        }
    } else if (!frame.bodyRan) {
        // Body was skipped: an existing live login was already resolved into
        // g_sec by cf_login_begin. Nothing to commit.
    } else {
        // Body ran with no <cfloginuser>: CF's doEndTag logs out.
        g_sec = SecurityContext{};
        string existing;
        string cookieName = authCookieName(appToken).c_str();
        if (!scopeString(static_cast<cfvariant*>(cookie), cookieName.constData(), existing) &&
            scope_context().loginStorageIsSession && session) {
            scopeString(static_cast<cfvariant*>(session), cookieName.constData(), existing);
        }
        if (!existing.isEmpty()) {
            security_remove_token(existing);
            if (scope_context().loginStorageIsSession) {
                if (session && static_cast<cfvariant*>(session)->m_type == cfvariant::Struct) {
                    static_cast<cfvariant*>(session)->m_struct->erase(string(cookieName.constData()));
                }
            } else {
                setAuthCookie(appToken, "", domain);
            }
        } else {
            setAuthCookie(appToken, "", domain);
        }
    }

    removeCfloginVar(static_cast<cfvariant*>(variables));
}

void cf_loginuser(string *out, void *cgi, void *server, void *cookie,
                  void *application, void *session, void *url, void *form,
                  void *variables,
                  const cfvariant *name, const cfvariant *password,
                  const cfvariant *roles)
{
    (void)out; (void)cgi; (void)server; (void)cookie; (void)application;
    (void)session; (void)url; (void)form; (void)variables;

    if (!name || !password || !roles) {
        throw webstrada::exception("cfloginuser", "Attribute validation error for cfloginUser.", "");
    }
    string n = const_cast<cfvariant*>(name)->toString();
    string p = const_cast<cfvariant*>(password)->toString();
    string r = const_cast<cfvariant*>(roles)->toString();

    // Parse the comma-delimited role list (CF's UserTag.setRoles:
    // StringTokenizer on ','; a space inside an element stays part of it).
    std::vector<string> roleList;
    if (!r.isEmpty()) {
        auto parts = r.split(',', false);
        for (auto &part : parts) {
            roleList.push_back(part);
        }
    }

    if (!g_loginFrames.empty()) {
        // Inside a <cflogin>: bind to the enclosing tag (committed at its end).
        LoginFrame &frame = g_loginFrames.back();
        frame.userSet = true;
        frame.name = n;
        frame.password = p;
        frame.roles = std::move(roleList);
    } else {
        // Outside a <cflogin>: log in directly, no cookie (CF UserTag with no
        // AuthenticateTag ancestor: fContext.setSecureTable).
        g_sec.loggedIn = true;
        g_sec.username = n;
        g_sec.password = p;
        g_sec.roles = std::move(roleList);
        g_sec.appToken = currentAppToken(nullptr).c_str();
    }
}

void cf_logout(string *out, void *cgi, void *server, void *cookie,
               void *application, void *session, void *url, void *form,
               void *variables,
               const cfvariant *sessionAttr, const cfvariant *applicationtoken)
{
    (void)out; (void)cgi; (void)server; (void)application;
    (void)url; (void)form; (void)variables;

    std::string appToken = currentAppToken(applicationtoken);
    std::string cookieName = authCookieName(appToken);

    // Resolve the current token (cookie or session key).
    string token;
    bool storeInSession = scope_context().loginStorageIsSession;
    if (storeInSession) {
        scopeString(static_cast<cfvariant*>(session), cookieName.c_str(), token);
    } else {
        scopeString(static_cast<cfvariant*>(cookie), cookieName.c_str(), token);
    }

    std::string sessionMode = "current";
    if (sessionAttr) {
        string v = const_cast<cfvariant*>(sessionAttr)->toString();
        sessionMode = std::string(v.constData(), v.length());
        std::string low;
        for (char c : sessionMode) low += (char)tolower((unsigned char)c);
        if (low != "all" && low != "others" && low != "current") {
            throw webstrada::exception("cflogout",
                "Attribute validation error for the logout tag.", "");
        }
        sessionMode = low;
    }

    // CF's SecurityScopeTracker.logout: processLogoutAuth then remove the pool
    // entry (not for "others") and clear the cookie.
    if (!token.isEmpty()) {
        if (sessionMode == "all") {
            security_remove_by_app_token(string(appToken.c_str()));
        } else if (sessionMode == "current") {
            security_remove_token(token);
        }
        // "others": keep the current token, drop everything else for the app.
        if (sessionMode == "others") {
            // This engine's pool is keyed by token; "others" would remove every
            // other token sharing the app token. The pool stores one row per
            // login; there is no enumeration API, so this is a no-op besides
            // keeping the current login (CF keeps the current one too).
        }
    }

    if (storeInSession) {
        if (session && static_cast<cfvariant*>(session)->m_type == cfvariant::Struct) {
            static_cast<cfvariant*>(session)->m_struct->erase(string(cookieName.c_str()));
        }
    } else {
        setAuthCookie(appToken, "", "");
    }

    // Clear the request's secure table (CF's finishLogoutProcesss:
    // setSecureTable(null)). For "others" the current login survives, so only
    // clear when the current token was removed.
    if (sessionMode != "others") {
        g_sec = SecurityContext{};
    }
}

// ---- Auth functions ----

cfvariant *cf_getauthuser()
{
    if (!g_sec.loggedIn || g_sec.username.isEmpty()) {
        auto *ret = new cfvariant("");
        return ret;
    }
    auto *ret = new cfvariant(g_sec.username);
    return ret;
}

cfvariant *cf_getuserroles()
{
    string roles;
    for (size_t i = 0; i < g_sec.roles.size(); i++) {
        if (i) roles.append(',');
        roles.append(g_sec.roles[i]);
    }
    auto *ret = new cfvariant(roles);
    return ret;
}

cfvariant *cf_isuserloggedin()
{
    return boolVariant(!g_sec.username.isEmpty());
}

// All comma-separated roles in `role` must be present (CF's
// UserUtils.cflogin_IsUserInRole: every group must be a key).
cfvariant *cf_isuserinrole(const cfvariant *role)
{
    if (!role) return boolVariant(false);
    string r = const_cast<cfvariant*>(role)->toString();
    std::vector<string> groups;
    if (!r.isEmpty()) {
        auto parts = r.split(',', false);
        for (auto &part : parts) groups.push_back(part);
    }
    if (groups.empty()) return boolVariant(false);
    for (const auto &g : groups) {
        bool found = false;
        for (const auto &have : g_sec.roles) {
            if (have.equals(g)) { found = true; break; }
        }
        if (!found) return boolVariant(false);
    }
    return boolVariant(true);
}

// Any comma-separated role in `rolelist` present (CF's UserUtils.isUserInAnyRole
// trims each element before testing).
cfvariant *cf_isuserinanyrole(const cfvariant *rolelist)
{
    if (!rolelist) return boolVariant(false);
    string r = const_cast<cfvariant*>(rolelist)->toString();
    std::vector<string> groups;
    if (!r.isEmpty()) {
        auto parts = r.split(',', false);
        for (auto &part : parts) groups.push_back(part.trimmed());
    }
    for (const auto &g : groups) {
        for (const auto &have : g_sec.roles) {
            if (have.equals(g)) return boolVariant(true);
        }
    }
    return boolVariant(false);
}

} // namespace cfml

/**
 * @file tag_application.cpp
 * @brief <cfapplication> tag and application/session store runtime implementations.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/scope_store.h>
#include <webstrada/string.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <unistd.h>

namespace {

using webstrada::cfvariant;
using webstrada::string;

thread_local cfml::ScopeContext g_scope;

static int64_t nowSeconds()
{
    return static_cast<int64_t>(std::time(nullptr));
}

static double cfAppTimeoutSeconds(const cfvariant *v, double fallback)
{
    if (!v) return fallback;
    cfvariant::cfvariantType t = v->m_type;
    if (t == cfvariant::Number) return std::max(0.0, (double)v->m_int) * 86400.0;
    if (t == cfvariant::Long) return std::max(0.0, (double)v->m_long) * 86400.0;
    if (t == cfvariant::Float || t == cfvariant::DateTime) return std::max(0.0, v->m_double) * 86400.0;
    if (t == cfvariant::String) {
        string s = const_cast<cfvariant*>(v)->toString().trimmed();
        if (s.contains(',')) {
            auto parts = s.split(',');
            double secs = 0.0;
            double mult[] = {86400.0, 3600.0, 60.0, 1.0};
            for (size_t i = 0; i < parts.size() && i < 4; i++) {
                const char *pc = parts[i].trimmed().constData();
                double n = pc ? std::strtod(pc, nullptr) : 0.0;
                secs += n * mult[i];
            }
            return std::max(0.0, secs);
        }
        const char *sc = s.constData();
        double n = sc ? std::strtod(sc, nullptr) : 0.0;
        return std::max(0.0, n) * 86400.0;
    }
    return fallback;
}

static bool cfScopeBool(const cfvariant *v, bool fallback)
{
    return cfml::cfmlBoolean(v, fallback);
}

static string cookieScopeValue(const cfvariant *cookie, const char *name)
{
    if (!cookie || cookie->m_type != cfvariant::Struct) return "";
    auto it = cookie->m_struct->find(name);
    if (it == cookie->m_struct->end()) return "";
    return it->second.toString();
}

} // namespace

namespace cfml {

string makeCfToken()
{
    unsigned char buf[8];
    std::srand((unsigned)std::time(nullptr) ^ (unsigned)getpid() ^ (unsigned)(uintptr_t)&buf);
    for (int i = 0; i < 8; i++) buf[i] = (unsigned char)(rand() % 256);
    char part[17];
    std::snprintf(part, sizeof(part), "%08x%08x",
                  ((unsigned)buf[0] << 24) | ((unsigned)buf[1] << 16) | ((unsigned)buf[2] << 8) | buf[3],
                  ((unsigned)buf[4] << 24) | ((unsigned)buf[5] << 16) | ((unsigned)buf[6] << 8) | buf[7]);

    unsigned char uuid[16];
    for (int i = 0; i < 16; i++) uuid[i] = (unsigned char)(rand() % 256);
    uuid[6] = (unsigned char)((uuid[6] & 0x0F) | 0x40);
    uuid[8] = (unsigned char)((uuid[8] & 0x3F) | 0x80);
    char hex[33] = {0};
    std::snprintf(hex, sizeof(hex),
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X",
        uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
        uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
    return string(part) + "-" + string(hex);
}

void setSessionCookies(const string &cfid, const string &token)
{
    std::time_t t = std::time(nullptr) + 30LL * 365LL * 24LL * 3600LL;
    struct tm tmVal;
    char buf[64];
    if (gmtime_r(&t, &tmVal)) {
        static const char *wday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        static const char *mon[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        std::snprintf(buf, sizeof(buf), "%s, %02d %s %04d %02d:%02d:%02d GMT",
            wday[tmVal.tm_wday], tmVal.tm_mday, mon[tmVal.tm_mon], tmVal.tm_year + 1900,
            tmVal.tm_hour, tmVal.tm_min, tmVal.tm_sec);
    } else {
        std::snprintf(buf, sizeof(buf), "%s", "Thu, 27 Jul 2056 05:33:59 GMT");
    }
    auto &r = cfml::response();
    webstrada::string c1 = "CFID=";
    c1 += cfid;
    c1 += "; Expires=";
    c1 += buf;
    c1 += "; Path=/; HttpOnly";
    webstrada::string c2 = "CFTOKEN=";
    c2 += token;
    c2 += "; Expires=";
    c2 += buf;
    c2 += "; Path=/; HttpOnly";
    r.cookies.push_back(std::string(c1.constData(), c1.length()));
    r.cookies.push_back(std::string(c2.constData(), c2.length()));
}

thread_local bool g_searchImplicitScopes = false;

// Per-request REQUEST scope (reset in scope_begin; cleared at request end by
// scope_end so one request's values never leak into the next).
thread_local cfvariant g_requestScope;

void scope_begin(ScopeStore *store, cfvariant *application, cfvariant *session)
{
    auto &sc = g_scope;
    sc = cfml::ScopeContext{};
    sc.store = store;
    sc.application = application;
    sc.session = session;
    g_searchImplicitScopes = false;
    g_requestScope = cfvariant(cfvariant::Struct);
    cf_cfoutputonly_set(false);
    silent_buf_clear();
    query_scope_clear();
    transaction_clear_all();
    stored_proc_clear();
    invoke_clear();
    import_paths_clear();
    zip_ctx_clear();
    cfml::locale_reset();
    security_reset();
}

ScopeContext &scope_context()
{
    return g_scope;
}

void scope_end()
{
    auto &sc = g_scope;
    int64_t now = nowSeconds();

    if (sc.store && sc.applicationEnabled && !sc.appName.empty() && sc.application &&
        sc.application->m_type == cfvariant::Struct) {
        int64_t expiresAt = (sc.appTimeoutSeconds > 0)
            ? now + static_cast<int64_t>(sc.appTimeoutSeconds) : 0;
        sc.store->storeApplication(sc.appName, scope_json_serialize(*sc.application), expiresAt, now);
    }

    if (sc.store && sc.sessionEnabled && !sc.sessionId.empty() && sc.session &&
        sc.session->m_type == cfvariant::Struct) {
        int64_t expiresAt = (sc.sessionTimeoutSeconds > 0)
            ? now + static_cast<int64_t>(sc.sessionTimeoutSeconds) : 0;
        sc.store->storeSession(sc.appName, sc.sessionId, scope_json_serialize(*sc.session), expiresAt, now, sc.sessionStartTime);
    }

    sc = cfml::ScopeContext{};
    g_requestScope = cfvariant(cfvariant::Struct);
}

cfvariant *cf_application_enable(cfvariant *application, cfvariant *session,
                                        const cfvariant *cookie,
                                        const cfvariant *name, const cfvariant *sessionManagement,
                                        const cfvariant *appTimeout, const cfvariant *sessionTimeout,
                                        const cfvariant *setClientCookies)
{
    auto &sc = g_scope;
    if (!sc.store || !application) {
        throw webstrada::exception("Application scope is not available in this context");
    }

    application->set_type(cfvariant::NotSet);
    application->set_type(cfvariant::Struct);
    application->m_disabled = false;

    string appName = name ? const_cast<cfvariant*>(name)->toString().trimmed() : string("");
    string storeKey = appName.isEmpty() ? string("<unnamed>") : appName;
    sc.appName = storeKey.constData();

    sc.appTimeoutSeconds = cfAppTimeoutSeconds(appTimeout, webstrada::config::defaultApplicationTimeoutSeconds);
    sc.sessionTimeoutSeconds = cfAppTimeoutSeconds(sessionTimeout, webstrada::config::defaultSessionTimeoutSeconds);
    sc.sessionManagement = cfScopeBool(sessionManagement, false);

    std::string json;
    bool found = sc.store->loadApplication(storeKey.constData(), nowSeconds(), json);
    if (found && scope_json_deserialize(json, *application)) {
        // loaded
    }
    application->m_disabled = false;
    sc.applicationEnabled = true;
    sc.appDirty = !found;

    if (sc.sessionManagement) {
        if (session) {
            session->set_type(cfvariant::NotSet);
            session->set_type(cfvariant::Struct);
            session->m_disabled = false;
        }
        string cfid = cookieScopeValue(cookie, "CFID");
        string token = cookieScopeValue(cookie, "CFTOKEN");
        string sessionId = cfid + ":" + token;
        bool haveIds = !cfid.isEmpty() && !token.isEmpty();

        std::string sjson;
        int64_t startTime = 0;
        bool foundSession = haveIds && sc.store->loadSession(storeKey.constData(), sessionId.constData(), nowSeconds(), sjson, &startTime);

        if (!foundSession) {
            int64_t newCfid = 1;
            sc.store->nextCfid(newCfid);
            token = makeCfToken();
            cfid = std::to_string(newCfid).c_str();
            sessionId = cfid;
            sessionId += ":";
            sessionId += token;
            if (cfScopeBool(setClientCookies, true)) {
                setSessionCookies(cfid, token);
            }
            sc.sessionNewlyCreated = true;
            sc.sessionStartTime = nowSeconds();
        } else if (session) {
            scope_json_deserialize(sjson, *session);
            session->m_disabled = false;
            sc.sessionStartTime = startTime;
        }

        sc.sessionId = sessionId.constData();
        sc.sessionEnabled = true;
    }

    return application;
}

void cf_set_search_implicit_scopes(const cfvariant *value)
{
    g_searchImplicitScopes = cfmlBoolean(value, false);
}

// <cfapplication loginstorage="session|cookie">: where the CFAUTHORIZATION
// login key is stored. "session" stores it in the session scope instead of a
// cookie (CF's ApplicationScope.setStoreloginCredentialInSession). Any other
// value (including the default "cookie") uses the cookie.
void cf_set_login_storage(const cfvariant *value)
{
    g_scope.loginStorageIsSession = false;
    if (!value) return;
    string v = const_cast<cfvariant*>(value)->toString();
    v.toLower();
    g_scope.loginStorageIsSession = v.equals("session");
}

} // namespace cfml

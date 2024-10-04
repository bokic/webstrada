/**
 * @file fn_csrf.cpp
 * @brief CFML csrfgeneratetoken() / csrfverifytoken() built-ins.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

namespace cfml {

namespace {

cfvariant *makeBoolResult(bool b) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = b;
    return ret;
}

// Session-side CSRF token store: CF keeps a private Map on the SessionScope
// object keyed by "<appName>_<key>". We mirror that in the session struct under
// a reserved key so it round-trips through the ScopeStore.
const char *kCsrfStoreKey = "__CF_CSRF_TOKENS__";

std::string sessionMapKey(const std::string &key) {
    auto &sc = scope_context();
    std::string app = sc.appName;
    if (!app.empty() && key.empty()) return app;
    return app + "_" + key;
}

// Uppercase hex of 20 random bytes (CF's CSRF token: 40 hex chars).
std::string randomToken() {
    unsigned char bytes[20];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
        throw webstrada::exception("CSRFGenerateToken", "Secure random number generation failed.", "");
    }
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char b : bytes) {
        out += hex[b >> 4];
        out += hex[b & 0x0F];
    }
    return out;
}

} // namespace

cfvariant *cf_csrfgeneratetoken(const cfvariant *key, const cfvariant *forceNew) {
    std::string k;
    if (key) {
        webstrada::string ks = const_cast<cfvariant*>(key)->toString();
        k = ks.constData() ? ks.constData() : "";
    }
    bool random = forceNew && cf_is_truthy_value(forceNew);

    auto &sc = scope_context();
    if (!sc.sessionEnabled || !sc.session) {
        throw webstrada::exception("CSRFTokenException",
                                  "The CSRF functions require session management to be enabled.", "");
    }
    cfvariant *session = sc.session;
    cfvariant *store = nullptr;
    auto it = session->m_struct->find(webstrada::string(kCsrfStoreKey));
    if (it != session->m_struct->end() && it->second.m_type == cfvariant::Struct) {
        store = const_cast<cfvariant*>(&it->second);
    }
    if (!store) {
        // Create the store struct in the session.
        cfvariant storeVal(cfvariant::Struct);
        session->structSet(kCsrfStoreKey, storeVal);
        store = const_cast<cfvariant*>(&session->m_struct->find(webstrada::string(kCsrfStoreKey))->second);
    }

    std::string mapKey = sessionMapKey(k);
    std::string token;
    auto tokIt = store->m_struct->find(webstrada::string(mapKey.c_str()));
    if (random || tokIt == store->m_struct->end()) {
        token = randomToken();
        store->structSet(mapKey.c_str(), cfvariant(token.c_str()));
    } else {
        webstrada::string existing = const_cast<cfvariant*>(&tokIt->second)->toString();
        token = existing.constData() ? existing.constData() : "";
    }
    sc.sessionDirty = true;
    return new cfvariant(token.c_str());
}

cfvariant *cf_csrfverifytoken(const cfvariant *token, const cfvariant *key) {
    if (!token) return makeBoolResult(false);
    webstrada::string tokStr = const_cast<cfvariant*>(token)->toString();
    std::string passed = tokStr.constData() ? tokStr.constData() : "";
    std::string k;
    if (key) {
        webstrada::string ks = const_cast<cfvariant*>(key)->toString();
        k = ks.constData() ? ks.constData() : "";
    }

    auto &sc = scope_context();
    if (!sc.sessionEnabled || !sc.session) {
        return makeBoolResult(false);
    }
    cfvariant *session = sc.session;
    auto it = session->m_struct->find(webstrada::string(kCsrfStoreKey));
    if (it == session->m_struct->end() || it->second.m_type != cfvariant::Struct) {
        return makeBoolResult(false);
    }
    std::string mapKey = sessionMapKey(k);
    auto tokIt = it->second.m_struct->find(webstrada::string(mapKey.c_str()));
    if (tokIt == it->second.m_struct->end()) {
        return makeBoolResult(false);
    }
    webstrada::string storedStr = const_cast<cfvariant*>(&tokIt->second)->toString();
    std::string stored = storedStr.constData() ? storedStr.constData() : "";

    // CF compares the first 40 characters of the stored token against the
    // first 40 of the passed token (or a DUMMY for short inputs), constant-time.
    auto first40 = [](const std::string &s) {
        std::string out;
        for (size_t i = 0; i < 40 && i < s.size(); i++) out += s[i];
        while (out.size() < 40) out += '0';
        return out;
    };
    std::string a = first40(stored);
    std::string b = first40(passed);
    // Constant-time compare.
    unsigned char diff = 0;
    for (size_t i = 0; i < 40; i++) diff |= (unsigned char)(a[i] ^ b[i]);
    return makeBoolResult(diff == 0);
}

} // namespace cfml

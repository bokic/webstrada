/**
 * @file fn_verifyclient.cpp
 * @brief CFML verifyClient() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <openssl/evp.h>
#include <cstdio>
#include <cstring>
#include <string>

using webstrada::cfvariant;
using webstrada::string;

namespace cfml {

cfvariant *cf_verifyclient() {
    auto &sc = scope_context();
    string requestClientId;
    bool foundInRequest = false;

    // 1. Check URL scope for _cf_clientid (case-insensitive)
    if (sc.url && sc.url->m_type == cfvariant::Struct && sc.url->m_struct) {
        for (const auto &[k, v] : *sc.url->m_struct) {
            if (k.compareCaseInsensitive("_cf_clientid") == 0) {
                requestClientId = const_cast<cfvariant&>(v).toString();
                foundInRequest = true;
                break;
            }
        }
    }

    // 2. If not in URL, check FORM scope for _cf_clientid (case-insensitive)
    if (!foundInRequest && sc.form && sc.form->m_type == cfvariant::Struct && sc.form->m_struct) {
        for (const auto &[k, v] : *sc.form->m_struct) {
            if (k.compareCaseInsensitive("_cf_clientid") == 0) {
                requestClientId = const_cast<cfvariant&>(v).toString();
                foundInRequest = true;
                break;
            }
        }
    }

    if (!foundInRequest) {
        throw webstrada::exception("Application", "Client verification failure.", "You must have a valid login to access this page.");
    }

    // 3. Construct expected client id from session or client urltoken
    string urltoken;
    if (sc.session && sc.session->m_type == cfvariant::Struct && sc.session->m_struct) {
        for (const auto &[k, v] : *sc.session->m_struct) {
            if (k.compareCaseInsensitive("urltoken") == 0) {
                urltoken = const_cast<cfvariant&>(v).toString();
                break;
            }
        }
    }
    if (urltoken.isEmpty() && sc.sessionEnabled && !sc.sessionId.empty()) {
        size_t colon = sc.sessionId.find(':');
        string cfid = (colon == std::string::npos) ? sc.sessionId.c_str() : sc.sessionId.substr(0, colon).c_str();
        string token = (colon == std::string::npos) ? "" : sc.sessionId.substr(colon + 1).c_str();
        urltoken = string("CFID=") + cfid + "&CFTOKEN=" + token;
    }

    if (!urltoken.isEmpty()) {
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digestLen = 0;
        EVP_MD_CTX *ctx = EVP_MD_CTX_new();
        if (ctx) {
            EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
            EVP_DigestUpdate(ctx, urltoken.constData(), urltoken.length());
            EVP_DigestFinal_ex(ctx, digest, &digestLen);
            EVP_MD_CTX_free(ctx);
        }
        char hexStr[33];
        for (unsigned int i = 0; i < digestLen && i < 16; i++) {
            snprintf(hexStr + i * 2, 3, "%02X", digest[i]);
        }
        hexStr[32] = '\0';
        string expectedClientId(hexStr);
        if (!requestClientId.equals(expectedClientId)) {
            throw webstrada::exception("Application", "Client verification failure.", "You must have a valid login to access this page.");
        }
    }

    return cfvariant_create_null();
}

} // namespace cfml

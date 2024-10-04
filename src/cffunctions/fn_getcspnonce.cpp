/**
 * @file fn_getcspnonce.cpp
 * @brief CFML getcspnonce() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_getcspnonce() {
    // CF only mints a nonce when a Content-Security-Policy is configured for
    // the request; otherwise GetCSPNonce() returns "" (verified against CF
    // 2025 on the RDS host, where Len(GetCSPNonce()) is 0).
    return new cfvariant("");
}

} // namespace cfml

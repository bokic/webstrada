/**
 * @file fn_islocalhost.cpp
 * @brief CFML islocalhost() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <string>

namespace cfml {

cfvariant *cf_islocalhost(const cfvariant *value) {
    if (!value) throw webstrada::exception("IsLocalHost requires exactly 1 argument");
    string host = const_cast<cfvariant*>(value)->toString();

    bool res = false;
    // CF compares the raw string (only the null/empty check trims; verified
    // against CF 2025: "  localhost  " is NO, "LOCALHOST" is YES).
    string lower = host;
    lower.toLower();
    if (lower.equals("localhost") || host.equals("127.0.0.1") ||
        host.equals("[::1]")) {
        res = true;
    } else if (host.contains(':')) {
        // IPv6 localhost: any IPv6 form that resolves to ::1 (CF's
        // isIPV6Localhost / IPAddressUtils._isLocalHost, verified against CF
        // 2025: ::1 and 0:0:0:0:0:0:0:1 -> YES; other loopback addresses -> NO).
        unsigned char lo[16] = {0};
        lo[15] = 1; // ::1
        bool match = false;
        struct in6_addr a6;
        if (inet_pton(AF_INET6, host.constData(), &a6) == 1) {
            match = (memcmp(a6.s6_addr, lo, 16) == 0);
        } else {
            struct addrinfo hints, *ai = nullptr;
            std::memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            if (getaddrinfo(host.constData(), nullptr, &hints, &ai) == 0) {
                for (struct addrinfo *rp = ai; rp; rp = rp->ai_next) {
                    if (rp->ai_family == AF_INET6) {
                        const struct sockaddr_in6 *sa =
                            reinterpret_cast<const struct sockaddr_in6*>(rp->ai_addr);
                        if (memcmp(sa->sin6_addr.s6_addr, lo, 16) == 0) { match = true; break; }
                    }
                }
                freeaddrinfo(ai);
            }
        }
        res = match;
    }

    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = res;
    return ret;
}

} // namespace cfml

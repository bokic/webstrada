/**
 * @file fn_isipv6.cpp
 * @brief CFML isipv6() built-in.
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

cfvariant *cf_isipv6(const cfvariant *value) {
    if (!value) throw webstrada::exception("IsIPv6 requires exactly 1 argument");
    string s = const_cast<cfvariant*>(value)->toString();
    string up = s;
    up.toUpper();

    // ColdFusion special-cases the loopback/hostname forms to "does this server
    // have IPv6" — true on the RDS host (verified against CF 2025). The
    // decompiled IPAddressUtils.isIPV6():
    //   localhost / 127.0.0.1 / [::1] / ::1 / 0:0:0:0:0:0:0:1 -> isIPV6()
    if (up.equals("LOCALHOST") || s.equals("127.0.0.1") || s.equals("[::1]") ||
        s.equals("::1") || s.equals("0:0:0:0:0:0:0:1")) {
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = true;
        return ret;
    }

    bool is6 = false;
    struct in6_addr a6;
    if (inet_pton(AF_INET6, s.constData(), &a6) == 1) {
        is6 = true;
        // IPv4-mapped IPv6 addresses (::ffff:0:0/96) resolve to an Inet4Address
        // in Java, so CF reports NO for them (verified: "::ffff:127.0.0.1" -> NO).
        static const unsigned char v4mapped[12] = {0,0,0,0,0,0,0,0,0,0,0xff,0xff};
        if (memcmp(a6.s6_addr, v4mapped, 12) == 0) is6 = false;
    } else {
        // Hostname resolution: any IPv6 result -> true (CF's
        // InetAddress.getAllByName + isIPv6Address). An empty string resolves
        // to the loopback, whose family is host-dependent.
        struct addrinfo hints, *res = nullptr;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        if (getaddrinfo(s.constData(), nullptr, &hints, &res) == 0) {
            for (struct addrinfo *rp = res; rp; rp = rp->ai_next) {
                if (rp->ai_family == AF_INET6) { is6 = true; break; }
            }
            freeaddrinfo(res);
        }
    }

    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = is6;
    return ret;
}

} // namespace cfml

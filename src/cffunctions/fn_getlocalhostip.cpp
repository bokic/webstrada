/**
 * @file fn_getlocalhostip.cpp
 * @brief CFML getlocalhostip() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_getlocalhostip() {
    // CF returns the loopback address (127.0.0.1 for IPv4, ::1 for IPv6);
    // the RDS host reports 127.0.0.1 (verified against CF 2025).
    return new cfvariant("127.0.0.1");
}

} // namespace cfml

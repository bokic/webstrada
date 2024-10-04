/**
 * @file fn_getcontextroot.cpp
 * @brief CFML getcontextroot() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_getcontextroot() {
    // CF returns the servlet context path; on a default ColdFusion installation
    // (and on the RDS host) that is the empty string (verified against CF
    // 2025). The engine has no separate web-app mount, so it reports "".
    return new cfvariant("");
}

} // namespace cfml

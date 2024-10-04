/**
 * @file fn_location.cpp
 * @brief CFML location() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

namespace cfml {

cfvariant *cf_location(const cfvariant *url, const cfvariant *addtoken,
                       const cfvariant *statuscode, int argCount) {
    (void)url; (void)addtoken; (void)statuscode;
    // ColdFusion 2025 removed the script-accessible Location() function (it is
    // only available as the <cflocation> tag); calling it throws CF's
    // method-not-found error with the argument count (verified on the RDS
    // host). The tag form keeps working via response_redirect.
    webstrada::string msg("Method Location with ");
    msg.append(webstrada::string::number(argCount));
    msg.append(" arguments is not in class coldfusion.runtime.CFPage.");
    throw webstrada::exception(msg);
}

} // namespace cfml

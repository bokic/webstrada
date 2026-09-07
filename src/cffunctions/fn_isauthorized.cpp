/**
 * @file fn_isauthorized.cpp
 * @brief CFML isAuthorized() built-in.
 *
 * isAuthorized(): NOT a ColdFusion 2025 function. It was part of the legacy
 * pre-MX Advanced Security framework removed in ColdFusion MX. CF reports
 * "Variable ISAUTHORIZED is undefined." (byte-verified on the RDS host).
 * The stub reproduces that error.
 */

#include "common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_isauthorized(const cfvariant *arg)
{
    (void)arg;
    throw webstrada::exception("Variable ISAUTHORIZED is undefined.");
}

} // namespace cfml

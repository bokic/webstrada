/**
 * @file fn_isauthenticated.cpp
 * @brief CFML isAuthenticated() built-in.
 *
 * isAuthenticated(): NOT a ColdFusion 2025 function. It was part of the legacy
 * pre-MX Advanced Security framework removed in ColdFusion MX. CF reports
 * "Variable ISAUTHENTICATED is undefined." (byte-verified on the RDS host).
 * The stub reproduces that error.
 */

#include "common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_isauthenticated(const cfvariant *arg)
{
    (void)arg;
    throw webstrada::exception("Variable ISAUTHENTICATED is undefined.");
}

} // namespace cfml

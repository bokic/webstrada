/**
 * @file fn_isprotected.cpp
 * @brief CFML isProtected() built-in.
 *
 * isProtected(): NOT a ColdFusion 2025 function. It was part of the legacy
 * pre-MX Advanced Security framework removed in ColdFusion MX. CF reports
 * "Variable ISPROTECTED is undefined." (byte-verified on the RDS host).
 * The stub reproduces that error.
 */

#include "common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_isprotected(const cfvariant *arg)
{
    (void)arg;
    throw webstrada::exception("Variable ISPROTECTED is undefined.");
}

} // namespace cfml

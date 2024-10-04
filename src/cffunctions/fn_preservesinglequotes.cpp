/**
 * @file fn_preservesinglequotes.cpp
 * @brief CFML preservesinglequotes() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <string>

namespace cfml {

cfvariant *cf_preservesinglequotes(const cfvariant *variable) {
    if (!variable) throw webstrada::exception("PreserveSingleQuotes requires exactly 1 argument");
    // Modern ColdFusion (2025) no longer auto-escapes single quotes in query
    // literals, so the function is an identity: the argument passes through
    // unchanged (verified against CF 2025 on the RDS host).
    return new cfvariant(const_cast<cfvariant*>(variable)->toString());
}

} // namespace cfml

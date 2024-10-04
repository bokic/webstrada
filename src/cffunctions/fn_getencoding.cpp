/**
 * @file fn_getencoding.cpp
 * @brief CFML getencoding() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

cfvariant *cf_getencoding(const cfvariant *scope) {
    if (!scope) throw webstrada::exception("GetEncoding requires exactly 1 argument");
    webstrada::string scopeName = const_cast<cfvariant*>(scope)->toString();
    scopeName.toUpper();
    if (!scopeName.equals("FORM") && !scopeName.equals("URL")) {
        throw webstrada::exception("Only form or URL scope is allowed in a SetEncoding() call.");
    }
    // The engine decodes both the FORM and URL scopes as UTF-8 per request
    // (verified against CF 2025 on the RDS host: GetEncoding("form") and
    // GetEncoding("url") both report UTF-8).
    return new cfvariant("UTF-8");
}

} // namespace cfml

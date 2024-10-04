/**
 * @file fn_setvariable.cpp
 * @brief CFML setvariable() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>

namespace cfml {

cfvariant *cf_setvariable(const cfvariant *name, const cfvariant *value,
                          void *cgi, void *server, void *cookie,
                          void *application, void *session, void *url,
                          void *form, void *variables) {
    if (!name || !value) throw webstrada::exception("SetVariable requires 2 arguments");
    string varName = const_cast<cfvariant*>(name)->toString();
    if (varName.isEmpty()) throw webstrada::exception("Variable name is required for SetVariable function.");
    cfvariant_assign(static_cast<cfvariant*>(cgi), static_cast<cfvariant*>(server),
                     static_cast<cfvariant*>(cookie), static_cast<cfvariant*>(application),
                     static_cast<cfvariant*>(session), static_cast<cfvariant*>(url),
                     static_cast<cfvariant*>(form), static_cast<cfvariant*>(variables),
                     varName.constData(), value);
    // CF 2025 returns the assigned value (verified on the RDS host:
    // #SetVariable("x", "v")# prints "v").
    return new cfvariant(*value);
}

} // namespace cfml

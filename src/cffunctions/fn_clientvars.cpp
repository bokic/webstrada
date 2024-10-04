/**
 * @file fn_clientvars.cpp
 * @brief CFML deleteclientvariable() / getclientvariableslist() built-ins.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_deleteclientvariable(const cfvariant *name) {
    (void)name;
    // The client scope is not implemented; on the RDS host client storage is
    // enabled, and CF's DeleteClientVariable returns true for any name once
    // the client scope exists (verified against CF 2025). See BUGS_CF.md.
    return cfvariant_create_bool(true);
}

cfvariant *cf_getclientvariableslist() {
    // No writable client variables exist (verified against CF 2025 on the RDS
    // host: an empty client scope reports the empty list).
    return new cfvariant("");
}

} // namespace cfml

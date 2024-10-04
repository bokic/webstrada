/**
 * @file fn_getfunctionlist.cpp
 * @brief CFML getfunctionlist() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_getfunctionlist() {
    // CF builds the map from its Java method names with empty-string values
    // (verified against CF 2025: StructCount is 800 and every value renders
    // empty). This engine enumerates its own built-in function registry, so
    // the exact key set/casing differs from CF (see BUGS_COSMETIC.md).
    cfvariant st(cfvariant::Struct);
    std::vector<std::string> names = builtinFunctionNames();
    for (const auto &n : names) {
        st.structSet(n.c_str(), cfvariant(""));
    }
    return new cfvariant(st);
}

} // namespace cfml

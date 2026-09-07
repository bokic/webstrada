/**
 * @file fn_getfunctioncalledname.cpp
 * @brief CFML GetFunctionCalledName() built-in.
 *
 * Returns the name of the currently executing function (the enclosing cffunction
 * or component method) as called at the call site. If called at template
 * level outside any function, returns an empty string "".
 */

#include "common.h"
#include "../core/core_internal.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>

#include <ranges>

namespace cfml {

cfvariant *cf_getfunctioncalledname() {
    for (const auto &ctx : g_udfCtx | std::views::reverse) {
        if (!ctx.calledName.empty()) {
            return new cfvariant(ctx.calledName.c_str());
        }
    }
    return new cfvariant("");
}

} // namespace cfml

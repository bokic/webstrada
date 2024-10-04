/**
 * @file fn_isdebugmode.cpp
 * @brief CFML isdebugmode() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_isdebugmode() {
    // Debug output is never enabled in this engine.
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    return ret;
}

} // namespace cfml

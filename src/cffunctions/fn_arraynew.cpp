/**
 * @file fn_arraynew.cpp
 * @brief CFML arraynew() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <vector>
#include <string>

namespace cfml {

cfvariant *cf_arraynew() {
    auto *ret = new cfvariant(cfvariant::Array);
    return ret;
}

} // namespace cfml

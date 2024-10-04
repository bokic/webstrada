/**
 * @file fn_structnew.cpp
 * @brief CFML structnew() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <set>
#include <vector>

namespace cfml {

cfvariant *cf_structnew() {
    auto *ret = new cfvariant(cfvariant::Struct);
    return ret;
}

} // namespace cfml

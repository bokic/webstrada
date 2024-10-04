/**
 * @file fn_getlocale.cpp
 * @brief CFML getlocale() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/locale.h>
#include <webstrada/string.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

namespace cfml {

cfvariant *cf_getlocale() {
    auto *ret = new cfvariant(currentLocaleStr());
    return ret;
}

} // namespace cfml

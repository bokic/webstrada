/**
 * @file fn_lsisnumeric.cpp
 * @brief CFML lsisnumeric() built-in.
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

cfvariant *cf_lsisnumeric(const cfvariant *val) {
    return cf_isnumeric(val);
}

} // namespace cfml

/**
 * @file fn_trim.cpp
 * @brief CFML trim() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace cfml {

cfvariant *cf_trim(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Trim requires exactly 1 argument");
    auto *ret = new cfvariant(const_cast<cfvariant*>(arg)->toString().trimmed());
    return ret;
}

} // namespace cfml

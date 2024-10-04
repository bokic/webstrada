/**
 * @file fn_decimalformat.cpp
 * @brief CFML decimalformat() built-in.
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

cfvariant *cf_decimalformat(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("DecimalFormat requires exactly 1 argument");
    double val = getDoubleValue(*arg);
    auto *ret = new cfvariant(formatDecimal(val));
    return ret;
}

} // namespace cfml

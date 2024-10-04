/**
 * @file fn_rematch.cpp
 * @brief CFML rematch() built-in.
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

cfvariant *cf_rematch(const cfvariant *regex, const cfvariant *str) {
    if (!regex || !str) throw webstrada::exception("REMatch requires exactly 2 arguments");
    webstrada::string re = variantToString(*regex);
    webstrada::string s = variantToString(*str);
    return doReMatch(re, s, false);
}

} // namespace cfml

/**
 * @file fn_rematchnocase.cpp
 * @brief CFML rematchnocase() built-in.
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

cfvariant *cf_rematchnocase(const cfvariant *regex, const cfvariant *str) {
    if (!regex || !str) throw webstrada::exception("REMatchNoCase requires exactly 2 arguments");
    webstrada::string re = variantToString(*regex);
    webstrada::string s = variantToString(*str);
    return doReMatch(re, s, true);
}

} // namespace cfml

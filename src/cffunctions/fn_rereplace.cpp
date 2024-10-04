/**
 * @file fn_rereplace.cpp
 * @brief CFML rereplace() built-in.
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

cfvariant *cf_rereplace(const cfvariant *str, const cfvariant *regex, const cfvariant *sub, const cfvariant *scopeVal) {
    if (!str || !regex || !sub) throw webstrada::exception("REReplace requires at least 3 arguments");
    webstrada::string s = variantToString(*str);
    webstrada::string re = variantToString(*regex);
    webstrada::string subStr = variantToString(*sub);
    webstrada::string scope = scopeVal ? variantToString(*scopeVal) : webstrada::string("one");
    return doReReplace(s, re, subStr, scope, false);
}

} // namespace cfml

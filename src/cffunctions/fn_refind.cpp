/**
 * @file fn_refind.cpp
 * @brief CFML refind() built-in.
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

cfvariant *cf_refind(const cfvariant *regex, const cfvariant *str, const cfvariant *startVal, const cfvariant *returnsubVal, const cfvariant *scopeVal) {
    if (!regex || !str) throw webstrada::exception("REFind requires at least 2 arguments");
    webstrada::string re = variantToString(*regex);
    webstrada::string s = variantToString(*str);
    int start = startVal ? getIntValue(*startVal) : 1;
    bool returnsub = returnsubVal ? isTruthy(*returnsubVal) : false;
    webstrada::string scope = scopeVal ? variantToString(*scopeVal) : webstrada::string("one");
    return doReFind(re, s, start, returnsub, scope, false);
}

} // namespace cfml

/**
 * @file fn_compare.cpp
 * @brief CFML compare() built-in.
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

cfvariant *cf_compare(const cfvariant *s1Val, const cfvariant *s2Val) {
    if (!s1Val || !s2Val) throw webstrada::exception("Compare requires exactly 2 arguments");
    webstrada::string s1 = const_cast<cfvariant*>(s1Val)->toString();
    webstrada::string s2 = const_cast<cfvariant*>(s2Val)->toString();
    int cmp = strcmp(s1.constData(), s2.constData());
    auto *ret = new cfvariant(cmp < 0 ? -1 : (cmp > 0 ? 1 : 0));
    return ret;
}

} // namespace cfml

/**
 * @file fn_lcase.cpp
 * @brief CFML lcase() built-in.
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

cfvariant *cf_lcase(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("LCase requires exactly 1 argument");
    webstrada::string s = const_cast<cfvariant*>(arg)->toString();
    s.toLower();
    auto *ret = new cfvariant(s);
    return ret;
}

} // namespace cfml

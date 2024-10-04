/**
 * @file fn_ucase.cpp
 * @brief CFML ucase() built-in.
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

cfvariant *cf_ucase(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("UCase requires exactly 1 argument");
    webstrada::string s = const_cast<cfvariant*>(arg)->toString();
    s.toUpper();
    auto *ret = new cfvariant(s);
    return ret;
}

} // namespace cfml

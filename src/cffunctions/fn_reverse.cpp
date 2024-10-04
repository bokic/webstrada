/**
 * @file fn_reverse.cpp
 * @brief CFML reverse() built-in.
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

cfvariant *cf_reverse(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("Reverse requires exactly 1 argument");
    webstrada::string s = const_cast<cfvariant*>(arg)->toString();
    webstrada::string rev;
    for (int i = s.length() - 1; i >= 0; i--) {
        rev += s.at(i);
    }
    auto *ret = new cfvariant(rev);
    return ret;
}

} // namespace cfml

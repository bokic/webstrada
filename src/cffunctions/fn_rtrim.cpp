/**
 * @file fn_rtrim.cpp
 * @brief CFML rtrim() built-in.
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

cfvariant *cf_rtrim(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("RTrim requires exactly 1 argument");
    webstrada::string s = const_cast<cfvariant*>(arg)->toString();
    int endPos = s.length();
    while (endPos > 0 && isspace(s.at(endPos - 1))) {
        endPos--;
    }
    auto *ret = new cfvariant(s.mid(0, endPos));
    return ret;
}

} // namespace cfml

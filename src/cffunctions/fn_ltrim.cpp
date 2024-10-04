/**
 * @file fn_ltrim.cpp
 * @brief CFML ltrim() built-in.
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

cfvariant *cf_ltrim(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("LTrim requires exactly 1 argument");
    webstrada::string s = const_cast<cfvariant*>(arg)->toString();
    int startPos = 0;
    while (startPos < s.length() && isspace(s.at(startPos))) {
        startPos++;
    }
    auto *ret = new cfvariant(s.mid(startPos, s.length() - startPos));
    return ret;
}

} // namespace cfml

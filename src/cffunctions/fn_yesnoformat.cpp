/**
 * @file fn_yesnoformat.cpp
 * @brief CFML yesnoformat() built-in.
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

cfvariant *cf_yesnoformat(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("YesNoFormat requires exactly 1 argument");
    auto *ret = new cfvariant(isTrue(*arg) ? "Yes" : "No");
    return ret;
}

} // namespace cfml

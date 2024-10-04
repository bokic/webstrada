/**
 * @file fn_htmleditformat.cpp
 * @brief CFML htmleditformat() built-in.
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

cfvariant *cf_htmleditformat(const cfvariant *str, const cfvariant *version) {
    if (!str) throw webstrada::exception("HTMLEditFormat requires at least 1 argument");
    auto *ret = new cfvariant(escapeHtmlEdit(variantToString(*str)));
    return ret;
}

} // namespace cfml

/**
 * @file fn_spanincluding.cpp
 * @brief CFML spanincluding() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/scope_store.h>
#include <webstrada/string.h>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>

using webstrada::cfvariant;
using webstrada::string;
using cfml::daysToTm;
using cfml::getIntValue;
using cfml::isTruthy;
using cfml::safe_to_std_string;
using cfml::variantToString;
using cfml::cfvariant_to_long;
using cfml::normalizeCharsetName;
using cfml::bytesToText;
using cfml::urlDecodeString;
using cfml::stringToBytes;
using cfml::getDaysOrThrow;
using cfml::tmToDays;
using cfml::cryptoHexDigits;

namespace cfml {

cfvariant *cf_spanincluding(const cfvariant *str, const cfvariant *set) {
    if (!str || !set) throw webstrada::exception("SpanIncluding requires exactly 2 arguments");
    string s = variantToString(*str);
    string setStr = variantToString(*set);
    string res;
    for (size_t i = 0; i < (size_t)s.length(); i++) {
        char c = s.at(static_cast<int>(i));
        if (setStr.indexOf(c) == -1) break;
        res.append(c);
    }
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml

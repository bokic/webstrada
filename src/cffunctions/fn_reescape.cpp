/**
 * @file fn_reescape.cpp
 * @brief CFML reescape() built-in.
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

cfvariant *cf_reescape(const cfvariant *str) {
    if (!str) throw webstrada::exception("ReEscape requires exactly 1 argument");
    webstrada::string s = const_cast<cfvariant*>(str)->toString();
    std::string text = s.constData() ? s.constData() : "";
    // ColdFusion escapes the set | $ ^ + ? { } * . [ ] ( ) \ & - (see
    // StringFunc.ESCAPE_REGEX_PATTERN). Note it does NOT escape '#' or '~'.
    const std::string reSpecial = "|$^+?{}*.[]()\\&-";
    std::string res;
    for (char c : text) {
        if (reSpecial.find(c) != std::string::npos) {
            res += '\\';
        }
        res += c;
    }
    auto *ret = new cfvariant(res.c_str());
    return ret;
}

} // namespace cfml

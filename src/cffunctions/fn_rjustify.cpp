/**
 * @file fn_rjustify.cpp
 * @brief CFML rjustify() built-in.
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

cfvariant *cf_rjustify(const cfvariant *str, const cfvariant *length) {
    if (!str || !length) throw webstrada::exception("RJustify requires exactly 2 arguments");
    webstrada::string s = const_cast<cfvariant*>(str)->toString();
    int targetLen = getIntValue(*length);
    int currentLen = (int)s.length();
    if (currentLen >= targetLen) {
        auto *ret = new cfvariant(s);
        return ret;
    }
    webstrada::string res;
    for (int i = 0; i < targetLen - currentLen; ++i) res.append(" ");
    res.append(s);
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml

/**
 * @file fn_removechars.cpp
 * @brief CFML removechars() built-in.
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

cfvariant *cf_removechars(const cfvariant *str, const cfvariant *start, const cfvariant *count) {
    if (!str || !start || !count) throw webstrada::exception("RemoveChars requires exactly 3 arguments");
    string s = variantToString(*str);
    int st = getIntValue(*start);
    int cnt = getIntValue(*count);
    if (st < 1 || st > s.length() || cnt < 0) {
        throw webstrada::exception("RemoveChars: Start index or count out of bounds");
    }
    string res = s;
    res.remove(st - 1, cnt);
    auto *ret = new cfvariant(res);
    return ret;
}

} // namespace cfml

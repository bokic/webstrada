/**
 * @file fn_week.cpp
 * @brief CFML week() built-in.
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

cfvariant *cf_week(const cfvariant *date) {
    if (!date) throw webstrada::exception("Week requires exactly 1 argument");
    double days = getDaysOrThrow(date, "Week");
    struct tm tm = daysToTm(days);
    struct tm jan1 = tm; jan1.tm_mon = 0; jan1.tm_mday = 1; jan1.tm_hour = 0; jan1.tm_min = 0; jan1.tm_sec = 0;
    int jan1_wday = daysToTm(tmToDays(jan1)).tm_wday;
    int val = (tm.tm_yday + jan1_wday) / 7 + 1;
    auto *ret = new cfvariant(val);
    return ret;
}

} // namespace cfml

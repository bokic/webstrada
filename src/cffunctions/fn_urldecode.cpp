/**
 * @file fn_urldecode.cpp
 * @brief CFML urldecode() built-in.
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

cfvariant *cf_urldecode(const cfvariant *str, const cfvariant *charset) {
    if (!str) throw webstrada::exception("URLDecode requires at least 1 argument");
    webstrada::string input = variantToString(*str);
    std::vector<std::byte> bytes;
    urlDecodeString(input, bytes, false);
    webstrada::string enc = charset ? const_cast<cfvariant*>(charset)->toString() : "UTF-8";
    webstrada::string out = bytesToText(bytes, enc);
    auto *ret = new cfvariant(out);
    return ret;
}

} // namespace cfml

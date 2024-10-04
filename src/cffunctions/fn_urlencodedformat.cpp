/**
 * @file fn_urlencodedformat.cpp
 * @brief CFML urlencodedformat() built-in.
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

cfvariant *cf_urlencodedformat(const cfvariant *str, const cfvariant *charset) {
    if (!str) throw webstrada::exception("URLEncodedFormat requires at least 1 argument");
    webstrada::string input = variantToString(*str);
    webstrada::string enc = charset ? const_cast<cfvariant*>(charset)->toString() : "UTF-8";
    std::vector<std::byte> bytes;
    stringToBytes(input, enc, bytes);
    webstrada::string out;
    for (auto b : bytes) {
        unsigned char c = static_cast<unsigned char>(b);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            out.append(static_cast<char>(c));
        } else {
            out.append('%');
            out.append(cryptoHexDigits[c >> 4]);
            out.append(cryptoHexDigits[c & 0xF]);
        }
    }
    auto *ret = new cfvariant(out);
    return ret;
}

} // namespace cfml

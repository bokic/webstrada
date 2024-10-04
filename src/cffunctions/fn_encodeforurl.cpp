/**
 * @file fn_encodeforurl.cpp
 * @brief CFML encodeforurl() built-in.
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

namespace {

static const char cryptoHexDigits[] = "0123456789ABCDEF";

} // namespace

namespace cfml {

cfvariant *cf_encodeforurl(const cfvariant *str, const cfvariant *canonicalize) {
    if (!str) throw webstrada::exception("EncodeForURL requires at least 1 argument");
    webstrada::string input = variantToString(*str);
    if (canonicalize && isTrue(*canonicalize)) {
        // ESAPI canonicalization: decode URL sequences before encoding.
        std::vector<std::byte> bytes;
        urlDecodeString(input, bytes, false);
        input = bytesToText(bytes, "UTF-8");
    }
    std::vector<std::byte> bytes;
    stringToBytes(input, "UTF-8", bytes);
    webstrada::string out;
    for (auto b : bytes) {
        unsigned char c = static_cast<unsigned char>(b);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.') {
            out.append(static_cast<char>(c));
        } else if (c == ' ') {
            out.append('+');
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

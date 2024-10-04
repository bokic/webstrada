/**
 * @file fn_val.cpp
 * @brief CFML val() built-in.
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

cfvariant *cf_val(const cfvariant *str) {
    if (!str) throw webstrada::exception("Val requires exactly 1 argument");
    // Number and Float inputs pass through unchanged (CF coerces to its numeric type).
    if (str->m_type == cfvariant::Number) {
        auto *ret = new cfvariant(str->m_int);
        return ret;
    }
    if (str->m_type == cfvariant::Float) {
        auto *ret = new cfvariant(cfvariant::Float);
        ret->m_double = str->m_double;
        return ret;
    }
    // Parse [whitespace][sign]digits[.digits]; parsing stops at the first
    // character that is not part of that grammar (no exponent support).
    webstrada::string s = variantToString(*str);
    int len = s.length();
    int i = 0;
    while (i < len && (s.at(i) == ' ' || s.at(i) == '\t' || s.at(i) == '\r' || s.at(i) == '\n')) i++;
    bool negative = false;
    if (i < len && (s.at(i) == '+' || s.at(i) == '-')) {
        negative = (s.at(i) == '-');
        i++;
    }
    double intPart = 0.0;
    bool hasIntDigits = false;
    while (i < len && s.at(i) >= '0' && s.at(i) <= '9') {
        intPart = intPart * 10.0 + static_cast<double>(s.at(i) - '0');
        hasIntDigits = true;
        i++;
    }
    double fracPart = 0.0;
    bool hasFracDigits = false;
    if (i < len && s.at(i) == '.') {
        i++;
        double scale = 0.1;
        while (i < len && s.at(i) >= '0' && s.at(i) <= '9') {
            fracPart += static_cast<double>(s.at(i) - '0') * scale;
            scale *= 0.1;
            hasFracDigits = true;
            i++;
        }
    }
    if (!hasIntDigits && !hasFracDigits) {
        auto *ret = new cfvariant(0);
        return ret;
    }
    double value = negative ? -(intPart + fracPart) : (intPart + fracPart);
    // CF returns an integer when the value is integral (trailing zeros do not
    // produce a float: Val("1.0") = 1) and a float otherwise.
    if (hasFracDigits && fracPart != 0.0) {
        auto *ret = new cfvariant(cfvariant::Float);
        ret->m_double = value;
        return ret;
    }
    if (value >= -2147483648.0 && value <= 2147483647.0) {
        auto *ret = new cfvariant(static_cast<int>(value));
        return ret;
    }
    // Values too large for a 32-bit integer are returned as floats.
    auto *ret = new cfvariant(cfvariant::Float);
    ret->m_double = value;
    return ret;
}

} // namespace cfml

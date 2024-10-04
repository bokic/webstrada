/**
 * @file fn_lsnumberformat.cpp
 * @brief CFML lsnumberformat() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/locale.h>
#include <webstrada/string.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

namespace cfml {

static long long roundHalfUp(double v)
{
    return static_cast<long long>((v < 0) ? (v - 0.5) : (v + 0.5));
}

static string formatNumberMask(double num, const string &mask, const cfml::LocaleInfo *loc)
{
    string m = mask.trimmed();
    if (m.isEmpty()) m = "_";
    bool negative = num < 0.0;
    double absv = std::abs(num);

    // Split on the last '.' (decimal point).
    int dot = m.lastIndexOf('.');
    string intMask = (dot < 0) ? m : m.left(dot);
    string fracMask = (dot < 0) ? "" : m.mid(dot + 1, m.length() - dot - 1);
    int fracDigits = 0;
    for (size_t i = 0; i < fracMask.length(); i++) {
        char c = fracMask.at(i);
        if (c == '9' || c == '0' || c == '#') fracDigits++;
    }
    int intWidth = 0;
    std::string zeroMask;
    for (size_t i = 0; i < intMask.length(); i++) {
        char c = intMask.at(i);
        if (c == '9' || c == '0' || c == '#') {
            zeroMask += (c == '0') ? '0' : ' ';
            intWidth++;
        }
    }
    bool hasGroup = intMask.indexOf(',') >= 0;

    // Round to fracDigits (half-up).
    double scale = 1.0;
    for (int i = 0; i < fracDigits; i++) scale *= 10.0;
    long long rounded = roundHalfUp(absv * scale);

    // Integer part (left of the decimal point after rounding).
    long long intPart = rounded / static_cast<long long>(scale);
    std::string digits = std::to_string(intPart);

    // Group the digits first (CF groups the digit run, not the padded width),
    // then right-align into intWidth; '0' mask positions zero-pad, others
    // space-pad.
    std::string grouped = hasGroup ? groupDigits(digits, loc->numGroupSep) : digits;
    int extra = intWidth - static_cast<int>(digits.length());
    std::string padded;
    if (extra > 0) {
        padded = zeroMask.substr(0, static_cast<size_t>(extra));
        padded += grouped;
    } else {
        padded = grouped;
    }

    if (negative) {
        // Insert the minus sign immediately before the first digit.
        size_t ins = padded.find_first_not_of(' ');
        if (ins == std::string::npos) ins = 0;
        padded.insert(ins, "-");
    }

    // Fraction part.
    std::string result = padded;
    if (fracDigits > 0) {
        long long fracPart = rounded % static_cast<long long>(scale);
        char fracBuf[32];
        std::snprintf(fracBuf, sizeof(fracBuf), "%0*lld", fracDigits, fracPart);
        result += loc->numDecSep;
        result += fracBuf;
    }
    return string(result.c_str());
}

static string formatNumberStandard(double num, const cfml::LocaleInfo *loc)
{
    long long rounded = roundHalfUp(std::abs(num));
    std::string digits = std::to_string(rounded);
    std::string grouped = groupDigits(digits, loc->numGroupSep);
    if (num < 0.0) grouped.insert(0, "-");
    return string(grouped.c_str());
}

cfvariant *cf_lsnumberformat(const cfvariant *number, const cfvariant *mask, const cfvariant *locale) {
    const cfml::LocaleInfo *loc = resolveLocale(locale);
    double num = lsNumberValue(number, "LSNumberFormat");
    if (mask && mask->m_type != cfvariant::Null) {
        string m = const_cast<cfvariant*>(mask)->toString();
        if (!m.trimmed().isEmpty()) {
            auto *ret = new cfvariant(formatNumberMask(num, m, loc));
            return ret;
        }
    }
    auto *ret = new cfvariant(formatNumberStandard(num, loc));
    return ret;
}

} // namespace cfml

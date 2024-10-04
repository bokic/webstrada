/**
 * @file fn_numberformat.cpp
 * @brief CFML numberformat() built-in.
 *
 * Port of coldfusion.runtime.locale.CFNumberFormat.LsNumberFormat for the
 * default (English) locale, byte-verified against ColdFusion 2025.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

namespace cfml {

namespace {

// Java Double.toString(): shortest round-trippable decimal. Plain decimal
// notation for |value| in [1e-3, 1e7), otherwise scientific "1.2345E7"
// (uppercase E, no '+', no leading zeros in the exponent, at least one digit
// after the mantissa point). Integral plain values get a trailing ".0".
std::string javaDoubleToStringFormat(double d) {
    if (std::isnan(d)) return "NaN";
    if (std::isinf(d)) return d > 0 ? "Infinity" : "-Infinity";
    if (d == 0.0) return "0.0"; // Java: 0.0, -0.0 -> "-0.0"
    bool neg = d < 0.0;
    if (neg) d = -d;

    // Shortest decimal representation (strtod round-trip).
    char buf[64];
    int prec = 1;
    for (; prec <= 17; prec++) {
        std::snprintf(buf, sizeof(buf), "%.*g", prec, d);
        if (std::strtod(buf, nullptr) == d) break;
    }
    if (prec > 17) std::snprintf(buf, sizeof(buf), "%.17g", d);
    std::string s(buf);

    // Parse into digits + decimal exponent.
    bool hasExp = false;
    std::string mant;
    int exp = 0;
    size_t epos = s.find_first_of("eE");
    if (epos != std::string::npos) {
        mant = s.substr(0, epos);
        exp = std::atoi(s.c_str() + epos + 1);
    } else {
        mant = s;
    }
    std::string digits;
    int pointPos = 0; // number of digits before the decimal point
    size_t dot = mant.find('.');
    if (dot == std::string::npos) {
        digits = mant;
        pointPos = (int)digits.size() + exp;
    } else {
        digits = mant.substr(0, dot) + mant.substr(dot + 1);
        pointPos = (int)dot + exp;
    }
    // Strip leading zeros, adjusting pointPos.
    size_t lead = digits.find_first_not_of('0');
    if (lead == std::string::npos) { digits = "0"; pointPos = 1; }
    else if (lead > 0) { digits = digits.substr(lead); pointPos -= (int)lead; }

    // Java: plain notation when 10^-3 <= value < 10^7, i.e. -2 <= pointPos <= 7.
    std::string out;
    if (pointPos >= -2 && pointPos <= 7) {
        if (pointPos <= 0) {
            out = "0.";
            for (int i = 0; i < -pointPos; i++) out += '0';
            out += digits;
        } else if (pointPos >= (int)digits.size()) {
            out = digits;
            for (int i = (int)digits.size(); i < pointPos; i++) out += '0';
            out += ".0";
        } else {
            out = digits.substr(0, pointPos) + "." + digits.substr(pointPos);
        }
    } else {
        // Scientific: d.dddE<exp> where exp = pointPos - 1.
        out = digits.substr(0, 1);
        if (digits.size() > 1) out += "." + digits.substr(1);
        else out += ".0";
        out += "E";
        out += std::to_string(pointPos - 1);
    }
    if (neg) out.insert(out.begin(), '-');
    return out;
}

// Parses a Java-style number string into (integerPart, decimalPart,
// hasDecimal). The string may contain an exponent which is resolved into a
// plain "digits with optional '.'" form like CFNumberFormat does.
struct PlainNumber {
    std::string integerPart;
    std::string decimalPart;
    std::string plain; // fully-expanded plain string (no exponent)
};

PlainNumber expandNumberString(const std::string &s) {
    PlainNumber pn;
    std::string str = s;
    size_t epos = str.find_first_of("eE");
    if (epos != std::string::npos) {
        int exp = std::atoi(str.c_str() + epos + 1);
        std::string mant = str.substr(0, epos);
        size_t dot = mant.find('.');
        std::string digits;
        int intLen;
        if (dot == std::string::npos) {
            digits = mant;
            intLen = (int)digits.size();
        } else {
            digits = mant.substr(0, dot) + mant.substr(dot + 1);
            intLen = (int)dot;
        }
        int absPos = intLen + exp;
        if (absPos <= 0) {
            std::string out = "0.";
            for (int i = 0; i < -absPos; i++) out += '0';
            out += digits;
            str = out;
        } else if (absPos >= (int)digits.size()) {
            std::string out = digits;
            for (int i = (int)digits.size(); i < absPos; i++) out += '0';
            str = out + ".0";
        } else {
            str = digits.substr(0, absPos) + "." + digits.substr(absPos);
        }
    }
    pn.plain = str;
    size_t d = str.find('.');
    if (d == std::string::npos) { pn.integerPart = str; pn.decimalPart = ""; }
    else { pn.integerPart = str.substr(0, d); pn.decimalPart = str.substr(d + 1); }
    return pn;
}

std::string groupDigitsStr(const std::string &digits, char sep) {
    std::string out;
    int n = (int)digits.size();
    for (int i = 0; i < n; i++) {
        if (i > 0 && (n - i) % 3 == 0) out += sep;
        out += digits[i];
    }
    return out;
}

} // namespace

cfvariant *cf_numberformat(const cfvariant *number, const cfvariant *mask) {
    if (!number) throw webstrada::exception("NumberFormat requires 1 argument");
    double num = lsNumberValue(number, "NumberFormat");

    // Default mask when absent: build a '9...9,' mask from the rounded integer
    // length (NumberFormatter.format(Object) logic).
    std::string maskStr;
    if (mask && mask->m_type != cfvariant::Null) {
        maskStr = const_cast<cfvariant*>(mask)->toString().trimmed().constData();
        if (maskStr.empty()) maskStr = "";
    }
    std::string effectiveMask = maskStr;

    if (effectiveMask.empty()) {
        long newnum = llround(num);
        std::string mask = std::to_string(newnum);
        int maskLength = (int)mask.size();
        std::string numStr = javaDoubleToStringFormat(num);
        size_t lastDot = numStr.find_last_of('.');
        if (maskLength > (int)lastDot && lastDot != std::string::npos) {
            maskLength = (int)mask.size() - 1;
        }
        std::string newMask;
        if (mask.find_first_of("eE") == std::string::npos) {
            for (int i = 0; i < maskLength; i++) {
                char c = mask[i];
                if (c != ',' && c != '.' && c != '-') newMask += '9';
                else if (c == '.') break;
                else newMask += c;
            }
        }
        newMask += ',';
        effectiveMask = newMask;
    }

    // ---- CFNumberFormat.LsNumberFormat port ----
    const char decimalPoint = '.';
    const char groupSeparator = ',';
    const char minusSign = '-';
    const std::string currencySymbol = "$";

    int maskLen = (int)effectiveMask.size();
    if (maskLen < 1) {
        throw webstrada::exception("zero length string", webstrada::string::number(num), "");
    }
    for (int i = 0; i < maskLen; i++) {
        if (std::string("_9.0()+-,LCR$^").find(effectiveMask[i]) == std::string::npos) {
            throw webstrada::exception("coldfusion.runtime.IllegalNumberFormatArgumentException",
                                      ("The mask " + effectiveMask + " is invalid for the input number: " + javaDoubleToStringFormat(num) + ".").c_str(), "");
        }
    }

    std::string String_number = javaDoubleToStringFormat(num);
    bool bNegativeNumber = false;
    if (num < 0.0) {
        String_number = String_number.substr(1);
        bNegativeNumber = true;
    }

    // ---- Rounding to the mask's fractional digit count (oldRoundingMethod=true
    // keeps the original CF behavior; the mask's right-digit count comes from
    // the legacy pre-round code below, so we round here like the "new" method
    // only when a decimal part is present and differs). ----
    // (The Java port relies on the fractional rounding block below; nothing to
    // do here.)

    // Exponent handling is inside expandNumberString.

    std::string String_number2 = String_number;
    PlainNumber pn = expandNumberString(String_number2);
    String_number2 = pn.plain;
    std::string String_integerNum = pn.integerPart;
    int numberDecimalPos = (int)String_number2.find('.');
    if (numberDecimalPos == (int)std::string::npos) numberDecimalPos = -1;
    if (numberDecimalPos > -1) String_integerNum = String_number2.substr(0, numberDecimalPos);

    int int_numberLen = (int)String_integerNum.size();
    std::string sign;
    int digitCountLeft = 0;
    int digitCountRight = 0;
    int zeropadright = 0;
    int plusSignPos = (int)effectiveMask.find('+');
    int nineDigitPos = (int)effectiveMask.find('9');
    int negSignPos = (int)effectiveMask.find('-');
    int underscorePos = (int)effectiveMask.find('_');
    int startparenPos = (int)effectiveMask.find('(');
    int dollarSignPos = (int)effectiveMask.find('$');
    int endparenPos = (int)effectiveMask.find(')');
    bool bSpacePadLeft = true;
    int decimalpos = (int)effectiveMask.find('.');
    int zeroPos = (int)effectiveMask.find('0');
    if (zeroPos >= 0 && (zeroPos < decimalpos || decimalpos == -1)) {
        bSpacePadLeft = false;
    }
    bool bThousandsSep = effectiveMask.find(',') != std::string::npos;
    bool bDollarSign = dollarSignPos >= 0;
    bool bUsePlus = plusSignPos >= 0;
    bool bUsePlusSpace = negSignPos >= 0;
    bool bUseParens = (startparenPos >= 0 || endparenPos >= 0);
    bool bDecimal = false;
    for (int i = 0; i < maskLen; i++) {
        if (effectiveMask[i] == '.') bDecimal = true;
        else if (!bDecimal) {
            char c2 = effectiveMask[i];
            if (c2 == '9' || c2 == '_' || c2 == '0') digitCountLeft++;
        } else {
            char c3 = effectiveMask[i];
            if (c3 == '9' || c3 == '_' || c3 == '0') { zeropadright++; digitCountRight++; }
        }
    }

    std::string decimalPart;
    if ((bDecimal && numberDecimalPos != -1) || !bDecimal) {
        if (numberDecimalPos != -1) decimalPart = String_number2.substr(numberDecimalPos + 1);
    } else {
        decimalPart = "";
    }

    std::string zeropadleft;
    int spacePadCount = 0;
    if (int_numberLen < digitCountLeft) {
        for (int i = 1; i <= digitCountLeft - int_numberLen; i++) {
            if (bSpacePadLeft) spacePadCount++;
            else zeropadleft += '0';
        }
    }
    std::string zeropadrightSB;
    if ((int)decimalPart.size() < digitCountRight) {
        for (int i = 1; i <= digitCountRight - (int)decimalPart.size(); i++) zeropadrightSB += '0';
    }

    bool bIncrementInteger = false;
    if ((int)decimalPart.size() > digitCountRight && digitCountRight > 0) {
        int truncatedDigit = decimalPart[digitCountRight] - '0';
        decimalPart = decimalPart.substr(0, digitCountRight);
        if (truncatedDigit >= 5) {
            bool bSigDigitAdded = false;
            if (decimalPart.size() > 0 && decimalPart[0] == '0') {
                decimalPart = "1" + decimalPart;
                bSigDigitAdded = true;
            }
            long long l = 0;
            if (!decimalPart.empty()) {
                try { l = std::stoll(decimalPart); } catch (...) { l = 0; }
            }
            int tmplen = (int)decimalPart.size();
            decimalPart = std::to_string(l + 1);
            if (bSigDigitAdded) decimalPart = decimalPart.substr(1);
            if (tmplen < (int)decimalPart.size()) {
                bIncrementInteger = true;
                decimalPart = decimalPart.substr(2) + "0";
            }
        }
        if ((int)decimalPart.size() >= digitCountRight) {
            decimalPart = decimalPart.substr(0, digitCountRight);
        }
    }

    if ((int)decimalPart.size() > 0 && (decimalPart[0] - '0') >= 5 && digitCountRight == 0) {
        long long TempInt = 0;
        try { TempInt = std::stoll(String_integerNum); } catch (...) { TempInt = 0; }
        String_integerNum = std::to_string(TempInt + 1);
    }
    if (bIncrementInteger) {
        long long TempInt = 0;
        try { TempInt = std::stoll(String_integerNum); } catch (...) { TempInt = 0; }
        String_integerNum = std::to_string(TempInt + 1);
    }

    if (digitCountRight - zeropadright > 0) {
        decimalPart = decimalPart.substr(0, digitCountRight - zeropadright);
    } else if ((digitCountRight != zeropadright) || ((digitCountRight == 0) && (zeropadright == 0))) {
        decimalPart = "";
    }

    std::string SB_numberString = String_integerNum;
    if (SB_numberString.size() > 0 && SB_numberString[0] == '-') {
        SB_numberString = SB_numberString.substr(1);
    }

    bool bSigned = false;
    if (!bNegativeNumber) {
        if (bUsePlusSpace) { bSigned = true; sign = " "; }
        else if (bUsePlus) { bSigned = true; sign = "+"; }
    } else if (bUsePlus || bUsePlusSpace) {
        bSigned = true;
        sign = "-";
    }
    if (bNegativeNumber && !bUseParens && !bUsePlus && !bUsePlusSpace) {
        bSigned = true;
        sign = "-";
    }

    bool bRightAlignSign = false;
    bool bRightAlignDollar = false;
    std::string strLeftFormatAndSign2;
    int intLeftSpaceCount = 0;
    int intRightSpaceCount = 0;
    if (spacePadCount > 0) {
        if (effectiveMask.find('C') != std::string::npos) {
            intLeftSpaceCount = (spacePadCount + 1) / 2;
            intRightSpaceCount = intLeftSpaceCount;
        } else if (effectiveMask.find('L') != std::string::npos) {
            intLeftSpaceCount = spacePadCount;
        } else {
            intRightSpaceCount = spacePadCount;
        }
    }
    std::string spacepadleftSB;
    std::string spacepadrightSB;
    for (int i = 1; i <= intRightSpaceCount; i++) spacepadleftSB += ' ';
    for (int i = 1; i <= intLeftSpaceCount; i++) spacepadrightSB += ' ';
    std::string strSpacingLeft = spacepadleftSB;
    std::string strSpacingRight = spacepadrightSB;

    if (underscorePos < dollarSignPos && startparenPos == -1) {
        bRightAlignDollar = true;
        if (underscorePos == -1 && ((plusSignPos > -1 || negSignPos > -1) &&
            (plusSignPos - dollarSignPos == 1 || dollarSignPos - plusSignPos == 1 ||
             negSignPos - dollarSignPos == 1 || dollarSignPos - negSignPos == 1))) {
            bRightAlignDollar = false;
        }
        if (dollarSignPos < nineDigitPos && nineDigitPos < decimalpos) {
            bRightAlignDollar = false;
        }
    } else if (underscorePos >= 0 && underscorePos < dollarSignPos && startparenPos >= 0) {
        bRightAlignDollar = true;
    }

    std::string strRightFormatAndSign;
    if (bSigned || bUseParens) {
        if (underscorePos != -1 && (underscorePos < plusSignPos || underscorePos < negSignPos || underscorePos < startparenPos)) {
            bRightAlignSign = true;
        }
        if (!bRightAlignSign) {
            if (nineDigitPos != -1 && (nineDigitPos < plusSignPos || nineDigitPos < negSignPos || nineDigitPos < startparenPos)) {
                bRightAlignSign = true;
            }
            if (nineDigitPos != -1 && plusSignPos == -1 && startparenPos == -1 && endparenPos == -1 && dollarSignPos == -1) {
                bRightAlignSign = true;
            }
        }
        int underscorePos2 = (int)effectiveMask.find_last_of('_');
        if (endparenPos < underscorePos2 && endparenPos != -1) {
            strRightFormatAndSign = ")" + strSpacingRight;
        } else if ((endparenPos > underscorePos2) && bNegativeNumber && sign.size() == 0) {
            strRightFormatAndSign = strSpacingRight + ")";
        } else {
            strRightFormatAndSign = strSpacingRight;
        }
    } else {
        strRightFormatAndSign = strSpacingRight;
    }

    if (!bSigned && bUseParens && bNegativeNumber) {
        strLeftFormatAndSign2 = "(";
        if (endparenPos < 0) strRightFormatAndSign = ")" + strSpacingRight;
    } else if (bSigned) {
        strLeftFormatAndSign2 = sign;
    }

    std::string strLeftFormatAndSign;
    if (bDollarSign) {
        if (bRightAlignDollar && bRightAlignSign) {
            strLeftFormatAndSign = strSpacingLeft + currencySymbol + strLeftFormatAndSign2;
        } else if (bRightAlignDollar && !bRightAlignSign) {
            strLeftFormatAndSign = strLeftFormatAndSign2 + strSpacingLeft + currencySymbol;
        } else if (!bRightAlignDollar && bRightAlignSign) {
            strLeftFormatAndSign = currencySymbol + strSpacingLeft + strLeftFormatAndSign2;
        } else {
            strLeftFormatAndSign = currencySymbol + strLeftFormatAndSign2 + strSpacingLeft;
        }
    } else if (bRightAlignSign) {
        strLeftFormatAndSign = strSpacingLeft + strLeftFormatAndSign2;
    } else {
        strLeftFormatAndSign = strLeftFormatAndSign2 + strSpacingLeft;
    }

    std::string numberString;
    if (bThousandsSep) {
        std::string tmp = zeropadleft + SB_numberString;
        zeropadleft.clear();
        // group from the right
        numberString = groupDigitsStr(tmp, groupSeparator);
    } else {
        numberString = SB_numberString;
    }

    std::string finalNumberString = strLeftFormatAndSign + zeropadleft + numberString;
    if (bDecimal) finalNumberString += std::string(1, decimalPoint) + decimalPart;
    finalNumberString += zeropadrightSB + strRightFormatAndSign;

    return new cfvariant(finalNumberString.c_str());
}

} // namespace cfml

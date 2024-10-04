/**
 * @file fn_wrap.cpp
 * @brief CFML wrap() built-in.
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

cfvariant *cf_wrap(const cfvariant *str, const cfvariant *limit, const cfvariant *strip) {
    if (!str || !limit) throw webstrada::exception("Wrap requires at least 2 arguments");
    webstrada::string s = const_cast<cfvariant*>(str)->toString();
    int maxLen = getIntValue(*limit);
    if (maxLen <= 0) throw webstrada::exception("Wrap: Limit must be a positive integer");
    bool doStrip = strip ? isTruthy(*strip) : false;

    std::string text = s.constData() ? s.constData() : "";
    if (doStrip) {
        std::string stripped;
        for (char c : text) {
            if (c == '\r' || c == '\n') {
                stripped.push_back(' ');
            } else {
                stripped.push_back(c);
            }
        }
        text = stripped;
    }

    std::string result;
    std::string currentLine;
    std::string currentWord;

    auto flushLine = [&]() {
        if (!result.empty()) result += "\n";
        result += currentLine;
        currentLine.clear();
    };

    for (size_t i = 0; i <= text.length(); ++i) {
        char c = (i < text.length()) ? text[i] : '\n';
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            if (!currentWord.empty()) {
                if (currentLine.empty()) {
                    currentLine = currentWord;
                } else if (currentLine.length() + 1 + currentWord.length() <= (size_t)maxLen) {
                    currentLine += " " + currentWord;
                } else {
                    flushLine();
                    currentLine = currentWord;
                }
                currentWord.clear();
            }
            if (c == '\n' || c == '\r') {
                flushLine();
                if (c == '\r' && i + 1 < text.length() && text[i + 1] == '\n') {
                    i++;
                }
            }
        } else {
            currentWord.push_back(c);
        }
    }
    if (!currentWord.empty()) {
        if (currentLine.empty()) {
            currentLine = currentWord;
        } else if (currentLine.length() + 1 + currentWord.length() <= (size_t)maxLen) {
            currentLine += " " + currentWord;
        } else {
            flushLine();
            currentLine = currentWord;
        }
    }
    if (!currentLine.empty()) {
        if (!result.empty()) result += "\n";
        result += currentLine;
    }

    auto *ret = new cfvariant(result.c_str());
    return ret;
}

} // namespace cfml

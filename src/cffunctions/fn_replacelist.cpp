/**
 * @file fn_replacelist.cpp
 * @brief CFML replacelist() built-in.
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

static string replaceAllInString(const string &s, const string &sub1, const string &sub2) {
    if (sub1.isEmpty()) return s;
    string res;
    int start = 0;
    while (start < s.length()) {
        int pos = s.indexOf(sub1, start);
        if (pos < 0) {
            res += s.mid(start, s.length() - start);
            break;
        }
        res += s.mid(start, pos - start);
        res += sub2;
        start = pos + sub1.length();
    }
    return res;
}

static std::vector<string> splitKeepEmpty(const string &s, const string &delim) {
    std::vector<string> res;
    if (delim.isEmpty()) {
        for (int i = 0; i < s.length(); i++) {
            string ch;
            ch.append(s.at(i));
            res.push_back(ch);
        }
        return res;
    }
    if (s.isEmpty()) {
        res.push_back(string(""));
        return res;
    }
    string cur;
    for (int i = 0; i < s.length(); i++) {
        char c = s.at(i);
        bool isDelim = false;
        for (int j = 0; j < delim.length(); j++) {
            if (c == delim.at(j)) {
                isDelim = true;
                break;
            }
        }
        if (isDelim) {
            res.push_back(cur);
            cur.clear();
        } else {
            cur.append(c);
        }
    }
    res.push_back(cur);
    return res;
}

cfvariant *cf_replacelist(const cfvariant *str, const cfvariant *list1, const cfvariant *list2,
                                const cfvariant *delim1, const cfvariant *delim2,
                                const cfvariant *includeEmptyFields) {
    if (!str || !list1 || !list2) throw webstrada::exception("ReplaceList requires at least 3 arguments");
    string s = variantToString(*str);
    string d1 = ",";
    string d2 = ",";
    bool incEmpty = false;

    // CFPage.ReplaceList: a 4th argument of "true"/"yes"/"false"/"no" selects
    // the includeEmptyFields behavior (with comma delimiters); any other value
    // is used as both delimiters. A 5th argument of "true"/"yes"/"false"/"no"
    // selects includeEmptyFields (with the 4th argument as both delimiters).
    if (delim1) {
        string d1low = variantToString(*delim1);
        d1low.toLower();
        if (d1low.equals("true") || d1low.equals("yes")) {
            incEmpty = true;
        } else if (!(d1low.equals("false") || d1low.equals("no"))) {
            d1 = variantToString(*delim1);
            d2 = d1;
        }
    }
    if (delim2) {
        string d2low = variantToString(*delim2);
        d2low.toLower();
        if (d2low.equals("true") || d2low.equals("yes")) {
            incEmpty = true;
            d2 = d1;
        } else if (!(d2low.equals("false") || d2low.equals("no"))) {
            d2 = variantToString(*delim2);
        }
    }
    if (includeEmptyFields) {
        incEmpty = isTruthy(*includeEmptyFields);
    }

    std::vector<string> tokens1 = splitKeepEmpty(variantToString(*list1), d1);
    std::vector<string> tokens2 = splitKeepEmpty(variantToString(*list2), d2);

    string result = s;
    size_t i = 0;
    size_t j = 0;
    while (i < tokens1.size()) {
        string r = tokens1[i];
        if (r.isEmpty()) {
            i++;
            continue;
        }
        string t;
        if (j < tokens2.size()) {
            t = tokens2[j];
        } else {
            t = "";
            incEmpty = true;
        }
        if (!incEmpty && t.isEmpty()) {
            j++;
            continue;
        }
        i++;
        j++;
        result = replaceAllInString(result, r, t);
    }
    auto *ret = new cfvariant(result);
    return ret;
}

} // namespace cfml

/**
 * @file fn_tostring.cpp
 * @brief CFML tostring() built-in.
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

cfvariant *cf_tostring(const cfvariant *value, const cfvariant *encoding) {
    if (!value) throw webstrada::exception("ToString requires at least 1 argument");
    webstrada::string out;
    if (value->m_type == cfvariant::String) {
        out = const_cast<cfvariant*>(value)->toString();
    } else if (value->m_type == cfvariant::Function) {
        out = const_cast<cfvariant*>(value)->toString();
    } else if (value->m_type == cfvariant::Number) {
        out = string::number(value->m_int);
    } else if (value->m_type == cfvariant::Long) {
        out = string::number(value->m_long);
    } else if (value->m_type == cfvariant::Float) {
        // CF ToString keeps the literal text of float literals (8.0 -> "8.0").
        if (value->m_literalText) {
            out = *value->m_literalText;
        } else {
            out = formatShortestDouble(value->m_double);
        }
    } else if (value->m_type == cfvariant::Boolean) {
        // CF ToString follows the literal/computed distinction: literal
        // booleans become true/false, computed ones YES/NO (BUGS.md #7).
        out = value->m_boolLiteral ? (value->m_bool ? "true" : "false")
                                   : (value->m_bool ? "YES" : "NO");
    } else if (value->m_type == cfvariant::DateTime) {
        out = const_cast<cfvariant*>(value)->toString();
    } else if (value->m_type == cfvariant::Binary) {
        if (!value->m_binary) throw webstrada::exception("ToString: invalid binary object");
        webstrada::string enc = encoding ? normalizeCharsetName(variantToString(*encoding)) : "UTF-8";
        out = bytesToText(*value->m_binary, enc);
    } else {
        throw webstrada::exception("Complex object types cannot be converted to simple values.");
    }
    auto *ret = new cfvariant(out);
    return ret;
}

} // namespace cfml

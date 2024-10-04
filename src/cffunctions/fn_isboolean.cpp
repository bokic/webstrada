/**
 * @file fn_isboolean.cpp
 * @brief CFML isboolean() built-in.
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

cfvariant *cf_isboolean(const cfvariant *val) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    if (val) {
        if (val->m_type == cfvariant::Boolean) {
            ret->m_bool = true;
        } else if (val->m_type == cfvariant::Number || val->m_type == cfvariant::Float) {
            ret->m_bool = true;
        } else if (val->m_type == cfvariant::String) {
            string s = const_cast<cfvariant*>(val)->toString();
            s.toLower();
            if (s.equals("true") || s.equals("false") || s.equals("yes") || s.equals("no") || s.equals("1") || s.equals("0")) {
                ret->m_bool = true;
            }
        }
    }
    return ret;
}

} // namespace cfml

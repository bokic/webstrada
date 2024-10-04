/**
 * @file fn_isclosure.cpp
 * @brief CFML isclosure() built-in.
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

cfvariant *cf_isclosure(const cfvariant *val) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    // A Function value is a built-in method handle, which CF reports as neither
    // a closure nor a custom function (verified against CF 2021: IsClosure(pi)
    // and IsCustomFunction(pi) are both NO). Anonymous closures carry a UDFInfo
    // with isClosure=true; named UDFs are custom functions but not closures.
    ret->m_bool = val && val->m_type == cfvariant::Function && val->m_udf && val->m_udf->isClosure;
    return ret;
}

} // namespace cfml

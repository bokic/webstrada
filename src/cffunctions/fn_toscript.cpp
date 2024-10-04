/**
 * @file fn_toscript.cpp
 * @brief CFML toscript() built-in.
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

static void toScriptEscape(const webstrada::string &input, webstrada::string &out) {
    for (int i = 0; i < input.length(); i++) {
        char c = input.at(i);
        switch (c) {
        case '\\': out.append("\\\\"); break;
        case '\'': out.append("\\'"); break;
        case '"': out.append("\\\""); break;
        case '\n': out.append("\\n"); break;
        case '\r': out.append("\\r"); break;
        case '\t': out.append("\\t"); break;
        default: out.append(c); break;
        }
    }
}

static void buildToScriptValue(const cfvariant &value, const webstrada::string &path, webstrada::string &out) {
    if (value.m_type == cfvariant::String) {
        out.append(path);
        out.append(" = \"");
        toScriptEscape(const_cast<cfvariant&>(value).toString(), out);
        out.append("\";\n");
    } else if (value.m_type == cfvariant::Number) {
        out.append(path);
        out.append(" = ");
        out.append(string::number(value.m_int));
        out.append(";\n");
    } else if (value.m_type == cfvariant::Long) {
        out.append(path);
        out.append(" = ");
        out.append(string::number(value.m_long));
        out.append(";\n");
    } else if (value.m_type == cfvariant::Float) {
        out.append(path);
        out.append(" = ");
        // CF ToScript keeps the literal text of float literals (8.0 -> "8.0").
        if (value.m_literalText) {
            out.append(*value.m_literalText);
        } else {
            out.append(formatShortestDouble(value.m_double));
        }
        out.append(";\n");
    } else if (value.m_type == cfvariant::Boolean) {
        out.append(path);
        out.append(value.m_bool ? " = true;\n" : " = false;\n");
    } else if (value.m_type == cfvariant::DateTime) {
        struct tm tm = daysToTm(value.m_double);
        out.append(path);
        out.append(" =  new Date(");
        out.append(string::number(tm.tm_year + 1900));
        out.append(", ");
        out.append(string::number(tm.tm_mon));
        out.append(", ");
        out.append(string::number(tm.tm_mday));
        out.append(", ");
        out.append(string::number(tm.tm_hour));
        out.append(", ");
        out.append(string::number(tm.tm_min));
        out.append(", ");
        out.append(string::number(tm.tm_sec));
        out.append(");\n");
    } else if (value.m_type == cfvariant::Array) {
        if (!value.m_array) throw webstrada::exception("ToScript: invalid array object");
        out.append(path);
        out.append(" =  new Array();\n");
        for (size_t i = 0; i < value.m_array->size(); i++) {
            webstrada::string elemPath = path;
            elemPath.append("[");
            elemPath.append(string::number(static_cast<int>(i)));
            elemPath.append("]");
            buildToScriptValue(value.m_array->at(i), elemPath, out);
        }
    } else if (value.m_type == cfvariant::Struct) {
        if (!value.m_struct) throw webstrada::exception("ToScript: invalid struct object");
        out.append(path);
        out.append(" = new Object();\n");
        for (const auto &pair : *value.m_struct) {
            webstrada::string keyName = pair.first;
            keyName.toLower(); // CF lowercases struct keys in WDDX/ToScript output
            webstrada::string keyPath = path;
            keyPath.append("[\"");
            toScriptEscape(keyName, keyPath);
            keyPath.append("\"]");
            buildToScriptValue(pair.second, keyPath, out);
        }
    } else {
        throw webstrada::exception("ToScript: value of type " + string::number(static_cast<int>(value.m_type)) + " cannot be converted to a JavaScript expression");
    }
}

cfvariant *cf_toscript(const cfvariant *cfvar, const cfvariant *javascriptvar, const cfvariant *outputformat, const cfvariant *asformat) {
    if (!cfvar || !javascriptvar) throw webstrada::exception("ToScript requires at least 2 arguments");
    // outputformat and asformat only affect WDDX/ActionScript container output in
    // CF; probes on CF 2021 show identical output for both settings, so they are
    // accepted but do not change the generated script.
    webstrada::string varName = variantToString(*javascriptvar);
    webstrada::string out;
    buildToScriptValue(*cfvar, varName, out);
    auto *ret = new cfvariant(out);
    return ret;
}

} // namespace cfml

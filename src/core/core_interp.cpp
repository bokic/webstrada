#include "core_internal.h"
#include "../cftags/common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
#include <webstrada/worker.h>
#include <webstrada/cfimage.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/scope_store.h>
#include <webstrada/config.h>
#include <webstrada/locale.h>
#include <webstrada/cfimage.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>

#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>

using namespace webstrada;
using namespace cfml;

// Registers a fresh heap result with the request temp-variant cleanup and
// returns it by value (the interpreter evaluates to value-semantics cfvariants,
// so the heap object would otherwise be orphaned). Never used on borrowed
// pointers into live containers.
static cfvariant tempReturn(cfvariant *v) {
    cf_register_temp(v);
    return *v;
}

// ---- #...# template-expression interpreter ----
struct FuncCall {
    string name;
    std::vector<string> args;
};

// Detects a named argument `name = value` at the top level of a call argument
// string (a bare identifier, a single top-level '=' — not '=='/'!='/...). When
// matched, returns true and fills nameOut/valueOut. Used by the interpreter
// call path so `f(b="x")` binds by parameter name (BUGS.md "Named arguments").
static bool splitNamedArg(const string &arg, string &nameOut, string &valueOut)
{
    string s = arg.trimmed();
    if (s.isEmpty()) return false;
    // First char must be an identifier start.
    char c0 = s.at(0);
    if (!(isalpha(static_cast<unsigned char>(c0)) || c0 == '_')) return false;
    size_t i = 0;
    while (i < s.length()) {
        char c = s.at(i);
        if (isalnum(static_cast<unsigned char>(c)) || c == '_') { i++; continue; }
        if (c == '=') break;
        return false;
    }
    if (i == 0 || i >= s.length()) return false;
    // Must be a single '=' (not ==, !=, <=, >=, +=, ...).
    if (s.at(i) != '=') return false;
    if (i + 1 < s.length() && s.at(i + 1) == '=') return false;
    // The '=' must be at the top level (no parens/brackets before it).
    nameOut = s.left(i).trimmed();
    valueOut = s.mid(i + 1, s.length() - i - 1).trimmed();
    if (valueOut.isEmpty()) return false;
    return true;
}

static bool parseFuncCall(const string &expr, FuncCall &call)
{
    string s = expr.trimmed();
    if (s.isEmpty() || s.at(s.length() - 1) != ')') return false;
    int firstParen = s.indexOf('(');
    if (firstParen <= 0) return false;

    call.name = s.left(firstParen).trimmed();
    string argsStr = s.mid(firstParen + 1, s.length() - firstParen - 2).trimmed();

    std::vector<string> parsedArgs;
    if (argsStr.isEmpty()) {
        call.args = parsedArgs;
        return true;
    }
    string curArg;
    int parenDepth = 0;
    bool inDoubleQuote = false;
    bool inSingleQuote = false;

    for (size_t i = 0; i < argsStr.length(); i++) {
        char c = argsStr.at(i);
        if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            curArg += c;
        } else if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            curArg += c;
        } else if ((c == '(' || c == '[' || c == '{') && !inDoubleQuote && !inSingleQuote) {
            parenDepth++;
            curArg += c;
        } else if ((c == ')' || c == ']' || c == '}') && !inDoubleQuote && !inSingleQuote) {
            parenDepth--;
            curArg += c;
        } else if (c == ',' && parenDepth == 0 && !inDoubleQuote && !inSingleQuote) {
            parsedArgs.push_back(curArg.trimmed());
            curArg.clear();
        } else {
            curArg += c;
        }
    }
    parsedArgs.push_back(curArg.trimmed());

    call.args = parsedArgs;
    return true;
}

std::vector<string> cfml::parseList(const string &listStr, const string &delim) {
    std::vector<string> res;
    if (listStr.isEmpty()) return res;

    string actualDelim = delim.isEmpty() ? "," : delim;
    string cur;
    for (int i = 0; i < listStr.length(); i++) {
        char c = listStr.at(i);
        bool isDelim = false;
        for (int j = 0; j < actualDelim.length(); j++) {
            if (c == actualDelim.at(j)) {
                isDelim = true;
                break;
            }
        }
        if (isDelim) {
            string trimmed = cur.trimmed();
            if (!trimmed.isEmpty()) {
                res.push_back(trimmed);
            }
            cur.clear();
        } else {
            cur += c;
        }
    }
    string trimmed = cur.trimmed();
    if (!trimmed.isEmpty()) {
        res.push_back(trimmed);
    }
    return res;
}

// Splits s on delim, ignoring delimiters nested inside (), [], {}, or quoted
// strings. Used for array/struct literal element splitting and for finding a
// top-level separator within a struct key:value pair.
static std::vector<string> splitTopLevel(const string &s, char delim)
{
    std::vector<string> res;
    if (s.isEmpty()) return res;

    string cur;
    int depth = 0;
    bool inDouble = false;
    bool inSingle = false;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s.at(i);
        if (c == '"' && !inSingle) {
            inDouble = !inDouble;
        } else if (c == '\'' && !inDouble) {
            inSingle = !inSingle;
        } else if (!inDouble && !inSingle) {
            if (c == '(' || c == '[' || c == '{') {
                depth++;
            } else if (c == ')' || c == ']' || c == '}') {
                depth--;
            } else if (c == delim && depth == 0) {
                res.push_back(cur.trimmed());
                cur.clear();
                continue;
            }
        }
        cur += c;
    }
    res.push_back(cur.trimmed());
    return res;
}

// Finds the index of the first top-level ':' or '=' that separates a struct
// literal key from its value, or -1 when none exists.
static int findTopLevelPairSep(const string &pair)
{
    int depth = 0;
    bool inDouble = false;
    bool inSingle = false;
    for (size_t i = 0; i < pair.length(); i++) {
        char c = pair.at(i);
        if (c == '"' && !inSingle) {
            inDouble = !inDouble;
        } else if (c == '\'' && !inDouble) {
            inSingle = !inSingle;
        } else if (!inDouble && !inSingle) {
            if (c == '(' || c == '[' || c == '{') {
                depth++;
            } else if (c == ')' || c == ']' || c == '}') {
                depth--;
            } else if (depth == 0 && (c == ':' || c == '=')) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}static string joinList(const std::vector<string> &elements, const string &delim) {
    string d = delim.isEmpty() ? "," : delim.first(1);
    string res;
    for (size_t i = 0; i < elements.size(); i++) {
        if (i > 0) res += d;
        res += elements[i];
    }
    return res;
}



double cfml::getDoubleValue(cfvariant v) {
    if (v.m_type == cfvariant::Number) return static_cast<double>(v.m_int);
    if (v.m_type == cfvariant::Long) return static_cast<double>(v.m_long);
    if (v.m_type == cfvariant::Float) return v.m_double;
    if (v.m_type == cfvariant::Boolean) return v.m_bool ? 1.0 : 0.0;
    // Bind the temporary to a named local: constData() on a temporary string
    // dangles once the temporary is destroyed (COW string), so numeric strings
    // computed on the fly (query-column cells, numbers, dates) would be parsed
    // from freed memory.
    string s = v.toString();
    const char *str = s.constData();
    if (!str) throw webstrada::exception("Parameter validation error: Expected a numeric value but received empty/null.");

    const char *parse_ptr = str;
    while (*parse_ptr && isspace(*parse_ptr)) parse_ptr++;
    if (*parse_ptr == '\0') {
        throw webstrada::exception("Parameter validation error: Expected a numeric value but received an empty string.");
    }

    char *end = nullptr;
    double d = strtod(parse_ptr, &end);
    while (end && *end && isspace(*end)) end++;

    if (end == parse_ptr || (end && *end != '\0')) {
        throw webstrada::exception("Parameter validation error: The value '" + v.toString() + "' cannot be converted to a number.");
    }
    return d;
}

int cfml::getIntValue(cfvariant v) {
    if (v.m_type == cfvariant::Number) return v.m_int;
    if (v.m_type == cfvariant::Long) return static_cast<int>(v.m_long);
    if (v.m_type == cfvariant::Float) return static_cast<int>(v.m_double);
    if (v.m_type == cfvariant::Boolean) return v.m_bool ? 1 : 0;
    string s = v.toString();
    const char *str = s.constData();
    if (!str) throw webstrada::exception("Parameter validation error: Expected a numeric value but received empty/null.");

    const char *parse_ptr = str;
    while (*parse_ptr && isspace(*parse_ptr)) parse_ptr++;
    if (*parse_ptr == '\0') {
        throw webstrada::exception("Parameter validation error: Expected a numeric value but received an empty string.");
    }

    char *end = nullptr;
    long val = strtol(parse_ptr, &end, 10);
    while (end && *end && isspace(*end)) end++;

    if (end == parse_ptr || (end && *end != '\0')) {
        throw webstrada::exception("Parameter validation error: The value '" + v.toString() + "' cannot be converted to an integer.");
    }
    return static_cast<int>(val);
}

// 64-bit integer conversion without truncation. Used by integer arithmetic so
// Long operands are not clamped to int32 (2147483648 + 1 == 2147483649).
long long cfml::getLongIntValue(cfvariant v) {
    if (v.m_type == cfvariant::Number) return v.m_int;
    if (v.m_type == cfvariant::Long) return v.m_long;
    if (v.m_type == cfvariant::Float) return static_cast<long long>(v.m_double);
    if (v.m_type == cfvariant::Boolean) return v.m_bool ? 1 : 0;
    string s = v.toString();
    const char *str = s.constData();
    if (!str) throw webstrada::exception("Parameter validation error: Expected a numeric value but received empty/null.");

    const char *parse_ptr = str;
    while (*parse_ptr && isspace(*parse_ptr)) parse_ptr++;
    if (*parse_ptr == '\0') {
        throw webstrada::exception("Parameter validation error: Expected a numeric value but received an empty string.");
    }

    char *end = nullptr;
    long long val = strtoll(parse_ptr, &end, 10);
    while (end && *end && isspace(*end)) end++;

    if (end == parse_ptr || (end && *end != '\0')) {
        throw webstrada::exception("Parameter validation error: The value '" + v.toString() + "' cannot be converted to an integer.");
    }
    return val;
}

bool cfml::cfvariantsEqual(const cfvariant &a, const cfvariant &b) {
    if (a.m_type == cfvariant::String || b.m_type == cfvariant::String) {
        string sa = const_cast<cfvariant&>(a).toString();
        string sb = const_cast<cfvariant&>(b).toString();
        return sa.equals(sb);
    }
    if (a.m_type == cfvariant::Number || b.m_type == cfvariant::Number ||
        a.m_type == cfvariant::Long || b.m_type == cfvariant::Long ||
        a.m_type == cfvariant::Float || b.m_type == cfvariant::Float) {
        try {
            return getDoubleValue(a) == getDoubleValue(b);
        } catch (...) {
            return false;
        }
    }
    if (a.m_type == cfvariant::Boolean && b.m_type == cfvariant::Boolean) {
        return a.m_bool == b.m_bool;
    }
    if (a.m_type == cfvariant::Null && b.m_type == cfvariant::Null) {
        return true;
    }
    return false;
}

bool cfml::cfvariantsEqualNoCase(const cfvariant &a, const cfvariant &b) {
    if (a.m_type == cfvariant::String || b.m_type == cfvariant::String) {
        string sa = const_cast<cfvariant&>(a).toString();
        string sb = const_cast<cfvariant&>(b).toString();
        return sa.compareCaseInsensitive(sb) == 0;
    }
    return cfvariantsEqual(a, b);
}

string cfml::variantToString(const cfvariant &v) {
    if (v.m_type == cfvariant::String) {
        return const_cast<cfvariant&>(v).toString();
    }
    if (v.m_type == cfvariant::Function) {
        return const_cast<cfvariant&>(v).toString();
    }
    if (v.m_type == cfvariant::Number) {
        return string::number(v.m_int);
    }
    if (v.m_type == cfvariant::Long) {
        return string::number(v.m_long);
    }
    if (v.m_type == cfvariant::Float) {
        if (v.m_literalText) return *v.m_literalText;
        return string::number(v.m_double);
    }
    if (v.m_type == cfvariant::Boolean) {
        return v.m_boolLiteral ? (v.m_bool ? "true" : "false") : (v.m_bool ? "YES" : "NO");
    }
    if (v.m_type == cfvariant::DateTime) {
        return const_cast<cfvariant&>(v).toString();
    }
    return "";
}

// Shortest round-trip decimal representation of a double (Java Double.toString
// style). Used by ToString/ToScript where CF prints full-precision values.
// NaN renders as the U+FFFD replacement character, matching CF 2021 (verified:
// ToString((-2)^0.5) yields a single U+FFFD character).

string cfml::formatDecimal(double val) {
    bool is_neg = (val < 0.0);
    double abs_val = std::abs(val);

    abs_val = std::round(abs_val * 100.0) / 100.0;

    double int_part;
    double frac_part = std::modf(abs_val, &int_part);

    long long int_val = static_cast<long long>(int_part);
    int frac_val = static_cast<int>(std::round(frac_part * 100.0));

    std::string int_str = std::to_string(int_val);
    std::string formatted_int;
    int len = int_str.length();
    for (int i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0) {
            formatted_int += ',';
        }
        formatted_int += int_str[i];
    }

    char frac_buf[16];
    std::snprintf(frac_buf, sizeof(frac_buf), "%02d", frac_val);

    std::string res;
    if (is_neg) res += '-';
    res += formatted_int;
    res += '.';
    res += frac_buf;

    return string(res.c_str());
}

string cfml::formatDollar(double val) {
    bool is_neg = (val < 0.0);
    double abs_val = std::abs(val);

    abs_val = std::round(abs_val * 100.0) / 100.0;

    double int_part;
    double frac_part = std::modf(abs_val, &int_part);

    long long int_val = static_cast<long long>(int_part);
    int frac_val = static_cast<int>(std::round(frac_part * 100.0));

    std::string int_str = std::to_string(int_val);
    std::string formatted_int;
    int len = int_str.length();
    for (int i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0) {
            formatted_int += ',';
        }
        formatted_int += int_str[i];
    }

    char frac_buf[16];
    std::snprintf(frac_buf, sizeof(frac_buf), "%02d", frac_val);

    std::string res;
    if (is_neg) {
        res += "($";
    } else {
        res += "$";
    }
    res += formatted_int;
    res += '.';
    res += frac_buf;
    if (is_neg) {
        res += ")";
    }

    return string(res.c_str());
}

bool cfml::isTrue(const cfvariant &val) {
    if (val.m_type == cfvariant::Boolean) {
        return val.m_bool;
    }
    if (val.m_type == cfvariant::Number) {
        return val.m_int != 0;
    }
    if (val.m_type == cfvariant::Long) {
        return val.m_long != 0;
    }
    if (val.m_type == cfvariant::Float) {
        return val.m_double != 0.0;
    }
    string s = const_cast<cfvariant&>(val).toString().trimmed();
    s.toLower();
    if (s.equals("true") || s.equals("yes") || s.equals("1")) {
        return true;
    }
    return false;
}


cfvariant cfml::callCallback(string &out, const cfvariant &callback, const std::vector<cfvariant> &args,
                       void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables)
{
    // A callable Function value (closure or UDF) is invoked directly.
    if (callback.m_type == cfvariant::Function && callback.m_udf && callback.m_udf->fn) {
        std::vector<const cfvariant*> argPtrs;
        argPtrs.reserve(args.size());
        for (const auto &a : args) argPtrs.push_back(&a);
        cfvariant *res = cfml::cf_udf_invoke(const_cast<cfvariant*>(&callback), argPtrs.data(),
                                             static_cast<int>(argPtrs.size()),
                                             out, cgi, server, cookie, application, session, url, form, variables);
        return *res;
    }
    string callbackName = const_cast<cfvariant&>(callback).toString();
    if (callbackName.isEmpty()) {
        throw webstrada::exception("Callback must be a non-empty string representing a function name");
    }

    cfvariant *vars = static_cast<cfvariant*>(variables);

    for (size_t tryCount = args.size(); tryCount >= 1; tryCount--) {
        for (size_t i = 0; i < tryCount; i++) {
            char keyBuf[64];
            std::snprintf(keyBuf, sizeof(keyBuf), "__CALLBACK_ARG_%zu", i);
            string key(keyBuf);
            vars->set(key) = args[i];
        }

        std::string stdExpr = callbackName.constData();
        stdExpr += "(";
        for (size_t i = 0; i < tryCount; i++) {
            if (i > 0) stdExpr += ",";
            char argBuf[64];
            std::snprintf(argBuf, sizeof(argBuf), "__CALLBACK_ARG_%zu", i);
            stdExpr += argBuf;
        }
        stdExpr += ")";

        string expr(stdExpr.c_str());

        try {
            cfvariant res = evaluateExpr(out, expr, cgi, server, cookie, application, session, url, form, variables);

            for (size_t i = 0; i < tryCount; i++) {
                char keyBuf[64];
                std::snprintf(keyBuf, sizeof(keyBuf), "__CALLBACK_ARG_%zu", i);
                string key(keyBuf);
                struct_data_bump(vars->m_structData);
                vars->m_struct->erase(key);
            }
            return res;
        } catch (const webstrada::exception &e) {
            for (size_t i = 0; i < tryCount; i++) {
                char keyBuf[64];
                std::snprintf(keyBuf, sizeof(keyBuf), "__CALLBACK_ARG_%zu", i);
                string key(keyBuf);
                struct_data_bump(vars->m_structData);
                vars->m_struct->erase(key);
            }

            string errMsg(e.what());
            errMsg.toUpper();
            if (tryCount == 1 || (!errMsg.contains("ARGUMENT") && !errMsg.contains("REQUIRES"))) {
                throw;
            }
        }
    }
    throw webstrada::exception("Callback invocation failed");
}



webstrada::cfvariant *cfml::lookupVarWritable(const char *name,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    if (!name || name[0] == '\0') return nullptr;

    struct { const char *n; void *p; } scopes[] = {
        {"VARIABLES", variables}, {"CGI", cgi}, {"URL", url},
        {"FORM", form}, {"COOKIE", cookie}, {"SERVER", server},
        {"APPLICATION", application}, {"SESSION", session},
        {"REQUEST", &g_requestScope},
    };

    webstrada::string key(name);

    // Key interning: the scope maps compare case-insensitively (CiLess), so the
    // name does not need to be uppercased for map finds. Only the scope-name
    // matching below needs a case-insensitive comparison. Split once on '.' for
    // dotted names; a simple name stays a single part without allocating.
    std::vector<string> parts;
    if (key.indexOf('.') < 0) {
        parts.push_back(key);
    } else {
        parts = key.split('.');
    }
    if (parts.empty()) return nullptr;

    // 1. Check if the first part is a scope name (e.g. "VARIABLES"). The THIS
    //    and LOCAL scopes are dynamic: they only exist inside a component
    //    method / UDF and resolve to the current call context.
    for (auto &s : scopes) {
        if (parts[0].compareCaseInsensitive(s.n) == 0) {
            // Inside a plain UDF, `variables` redirects to the calling page's
            // variables scope (was BUGS.md "UDF: variables.foo").
            void *scopePtr = s.p;
            if (s.n[0] == 'V' && !g_udfCtx.empty() &&
                static_cast<webstrada::cfvariant*>(s.p) == g_udfCtx.back().localScope) {
                scopePtr = udfVariablesScope(static_cast<webstrada::cfvariant*>(s.p));
            }
            return descendDottedPath(static_cast<webstrada::cfvariant*>(scopePtr), parts, 1);
        }
    }
    if (parts[0].compareCaseInsensitive("THIS") == 0) {
        for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
            if (it->component) {
                // CF's `this` is the component object itself (its members are
                // the this scope). Return a temp component so `this.x` and a
                // bare `this` (cfreturn this) both work.
                if (parts.size() == 1) {
                    cfvariant *comp = componentThisValue(it->component);
                    return comp;
                }
                return descendDottedPath(it->thisScope, parts, 1);
            }
        }
        return nullptr;
    }
    if (parts[0].compareCaseInsensitive("SUPER") == 0) {
        for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
            if (it->component && it->component->info) {
                ComponentInfo *targetInfo = it->componentInfo ? it->componentInfo->parent : it->component->info->parent;
                if (!targetInfo) {
                    throw webstrada::exception("component", "Component has no parent component (extends nothing).");
                }
                cfvariant *superVal = cf_component_get_super_scope();
                if (parts.size() == 1) {
                    return superVal;
                }
                return descendDottedPath(superVal, parts, 1);
            }
        }
        return nullptr;
    }
    if (parts[0].compareCaseInsensitive("LOCAL") == 0) {
        if (g_udfCtx.empty()) return nullptr;
        return descendDottedPath(g_udfCtx.back().localScope, parts, 1);
    }
    if (parts[0].compareCaseInsensitive("CALLER") == 0) {
        if (g_customTagStack.empty() || !g_customTagStack.back().callerVariables) return nullptr;
        if (parts.size() == 1) return g_customTagStack.back().callerVariables;
        return descendDottedPath(g_customTagStack.back().callerVariables, parts, 1);
    }
    if (parts[0].compareCaseInsensitive("ATTRIBUTES") == 0) {
        if (g_customTagStack.empty()) return nullptr;
        if (parts.size() == 1) return &g_customTagStack.back().attributes;
        return descendDottedPath(&g_customTagStack.back().attributes, parts, 1);
    }
    if (parts[0].compareCaseInsensitive("THISTAG") == 0) {
        if (g_customTagStack.empty()) return nullptr;
        if (parts.size() == 1) return &g_customTagStack.back().thisTag;
        return descendDottedPath(&g_customTagStack.back().thisTag, parts, 1);
    }

    // 2. Otherwise, check each scope in order to see if it has the first part
    //    as a member, and then traverse any sub-components. Inside a UDF /
    //    component method the function-local scope is searched FIRST, then the
    //    (component) variables scope, then the enclosing UDF parent scopes,
    //    then the component's this scope, then the fixed implicit scopes.
    auto tryScope = [&](webstrada::cfvariant *scope) -> webstrada::cfvariant* {
        if (scope && scope->m_type == webstrada::cfvariant::Struct && !scope->m_disabled) {
            auto it = scope->m_struct->find(parts[0]);
            if (it != scope->m_struct->end()) {
                return descendDottedPath(&it->second, parts, 1);
            }
        }
        return nullptr;
    };

    if (!g_udfCtx.empty()) {
        // The active UDF local scope has precedence even when its temporary
        // struct is marked disabled during component dispatch; query columns
        // must never replace a live local loop index.
        auto *local = g_udfCtx.back().localScope;
        webstrada::string localName = parts[0];
        localName.toUpper();
        auto loopIt = g_udfCtx.back().loopIndices.find(localName);
        if (loopIt == g_udfCtx.back().loopIndices.end()) {
            // The runtime string comparator is intentionally case-insensitive
            // for CF scopes, but a few containers can retain the original key
            // spelling.  Preserve CF lookup semantics by checking the active
            // loop bindings case-insensitively as a fallback.
            for (auto it = g_udfCtx.back().loopIndices.begin();
                 it != g_udfCtx.back().loopIndices.end(); ++it) {
                if (it->first.compareCaseInsensitive(parts[0]) == 0) {
                    loopIt = it;
                    break;
                }
            }
        }
        if (loopIt != g_udfCtx.back().loopIndices.end()) {
            return &loopIt->second;
        }
        if (local && local->m_type == webstrada::cfvariant::Struct && local->m_structData) {
            // StructData is the owning storage. Use its map directly instead
            // of the cached m_struct alias: local scopes can be copied while
            // component methods are materialized, leaving that alias stale.
            auto &localMap = local->m_structData->map;
            auto it = localMap.find(parts[0]);
            if (it != localMap.end()) {
                return descendDottedPath(&it->second, parts, 1);
            }

            if (g_udfCtx.back().localNames.find(localName) != g_udfCtx.back().localNames.end()) {
                auto inserted = localMap.emplace(parts[0], webstrada::cfvariant());
                return descendDottedPath(&inserted.first->second, parts, 1);
            }
        }
        // CF: arguments are not keys of the `local` scope; an unqualified name
        // that is a parameter resolves through the `arguments` scope (which the
        // local scope references as "ARGUMENTS"). A missing parameter is a Null
        // slot there and still reads as undefined (CF: "Variable A is
        // undefined.").
        if (cfvariant *args = udfArgumentsScope(g_udfCtx.back().localScope)) {
            if (args && args->m_type == cfvariant::Struct && !args->m_disabled) {
                auto it = args->m_struct->find(parts[0]);
                if (it != args->m_struct->end() && it->second.m_type != cfvariant::Null) {
                    return descendDottedPath(&it->second, parts, 1);
                }
            }
        }
    }
    // A compiled include has a separate template entry function and therefore
    // cannot rely on the caller's UDF stack frame. Its caller-local scope is
    // carried explicitly by IncludeRuntime. It belongs below every active UDF
    // frame: a component method called from an included template must resolve
    // its own `arguments`/locals before consulting the include's locals.
    if (auto *rt = cfml::include_context(); rt && rt->includeLocalScope) {
        if (auto *r = tryScope(rt->includeLocalScope)) return r;
    }
    // A component/UDF invoked from a custom tag still owns the normal local
    // scope precedence.  The custom tag's private variables apply only while
    // resolving the custom-tag template itself; they must not shadow a callee
    // local such as Queue.getElements()'s loop index.
    if (g_customTagExecutionVariables) {
        if (auto *r = tryScope(g_customTagExecutionVariables)) return r;
    }
    // Query columns are implicit variables. They must not shadow the current
    // function's local variables or arguments: MangoBlog has a query column
    // named `i`, while Queue.cfc uses a local loop index with that name.
    // After the active UDF scopes, query columns precede the enclosing
    // variables scope and the innermost query scope wins.
    if (auto *r = query_scope_resolve_member(parts)) return r;
    if (auto *r = tryScope(static_cast<webstrada::cfvariant*>(variables))) return r;
    if (!g_customTagStack.empty()) {
        if (auto *r = tryScope(&g_customTagStack.back().attributes)) return r;
    }
    for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
        if (auto *r = tryScope(it->parentScope)) return r;
        // A closure / nested function captures the enclosing function's local
        // scope; its parameters live in that scope's `arguments` struct (not
        // as local keys), so fall through to it too.
        if (auto *r = tryScope(udfArgumentsScope(it->parentScope))) return r;
    }
    // The innermost enclosing component method's this scope (also covers a
    // closure nested inside a method).
    for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
        if (it->thisScope) {
            if (auto *r = tryScope(it->thisScope)) return r;
            break;
        }
    }
    // An unqualified name is only searched in the variables scope (done above,
    // including the enclosing UDF parent scopes) and — when searchimplicitscopes
    // is enabled — the implicit scopes. SERVER / APPLICATION / SESSION are NEVER
    // searched for unqualified names (matches ColdFusion's searchScopes order).
    if (g_searchImplicitScopes) {
        // CF's implicit-scope search order: CGI, FILE, URL, FORM, COOKIE, CLIENT.
        // FILE / CLIENT are not implemented; the rest are searched in order.
        struct { const char *n; void *p; } implicit[] = {
            {"CGI", cgi}, {"URL", url}, {"FORM", form}, {"COOKIE", cookie},
        };
        for (auto &s : implicit) {
            if (auto *r = tryScope(static_cast<webstrada::cfvariant*>(s.p))) return r;
        }
    }

    return nullptr;
}

// Returns the root variable name of an expression for undefined-variable error
// reporting: strips a scope prefix (variables.x -> x), the first dotted member
// (s.key -> s) and the first array index (arr[1] -> arr). For a fully missing
// variable this is what ColdFusion names in its "Variable X is undefined."
// message.
static string extractRootVarName(const string &expr)
{
    string t = expr.trimmed();
    if (t.isEmpty()) return t;

    int dot = t.indexOf('.');
    int bracket = t.indexOf('[');
    int cut;
    if (dot < 0) cut = bracket;
    else if (bracket < 0) cut = dot;
    else cut = std::min(dot, bracket);
    string root = (cut < 0) ? t : t.left(cut).trimmed();

    // A scope-qualified name (variables.x, url.x, ...) is reported by its
    // member, matching ColdFusion's message for the missing variable itself.
    string head = root;
    head.toUpper();
    static const char *scopes[] = {"VARIABLES","CGI","URL","FORM","COOKIE","SERVER",
        "APPLICATION","SESSION","REQUEST","CLIENT","THIS","ARGUMENTS","LOCAL","SUPER",
        "CFLOCATION", nullptr};
    bool isScope = false;
    for (int i = 0; scopes[i]; i++) {
        if (head.equals(scopes[i])) { isScope = true; break; }
    }
    if (isScope && cut >= 0 && cut + 1 < t.length()) {
        string rest = t.mid(cut + 1, t.length() - cut - 1).trimmed();
        int d2 = rest.indexOf('.');
        int b2 = rest.indexOf('[');
        int c2;
        if (d2 < 0) c2 = b2;
        else if (b2 < 0) c2 = d2;
        else c2 = std::min(d2, b2);
        root = (c2 < 0) ? rest : rest.left(c2).trimmed();
    }
    return root;
}

// Returns the last dotted member name of an expression (s.key -> key), used
// when the root variable exists but a member lookup failed.
static string extractLastMember(const string &expr)
{
    string t = expr.trimmed();
    int lastDot = t.lastIndexOf('.', t.length() - 1);
    if (lastDot > 0 && lastDot + 1 < t.length()) {
        return t.mid(lastDot + 1, t.length() - lastDot - 1).trimmed();
    }
    return t;
}


cfvariant evaluateExpr(string &out, const string &expr,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables,
    bool parseBinary);

// Full binary/unary operator parser for #...# template expressions. Mirrors the
// CFML precedence table in llvm_codegen.cpp getOpPrecedence. sharpParsePrimary uses
// parseBinary=false to evaluate a single atom (a literal, variable, function
// call, member chain, or array index) without re-entering the operator parser.
static cfvariant sharpParseExpr(string &out, const string &s, size_t &pos, int minPrec,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables);
static cfvariant sharpParseUnary(string &out, const string &s, size_t &pos, int minPrec,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables);
static cfvariant sharpParsePrimary(string &out, const string &s, size_t &pos,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables);

// Finds the index of the bracket/paren/brace that closes the one opened at
// openPos, skipping over quoted strings and nested brackets/parens/braces of
// the same kind. Returns -1 when no matching close exists.
static int findMatchingClose(const string &s, int openPos)
{
    if (openPos < 0 || openPos >= s.length()) return -1;
    char open = s.at(openPos);
    char close = (open == '[') ? ']' : (open == '{') ? '}' : ')';
    int depth = 0;
    bool inDouble = false;
    bool inSingle = false;
    for (int i = openPos; i < s.length(); i++) {
        char c = s.at(i);
        if (c == '"' && !inSingle) {
            inDouble = !inDouble;
        } else if (c == '\'' && !inDouble) {
            inSingle = !inSingle;
        } else if (!inDouble && !inSingle) {
            if (c == open) {
                depth++;
            } else if (c == close) {
                depth--;
                if (depth == 0) return i;
            }
        }
    }
    return -1;
}

// Detects a trailing member-access chain ('.key' / '[expr]') after a base
// expression and returns the index where the chain begins, or -1 when the
// whole expression is the base. The first top-level '(' or '[' marks the end
// of the base; the matching close is found with findMatchingClose().
static int findChainStart(const string &e)
{
    int firstOpen = e.indexOf('(');
    int firstBracket = e.indexOf('[');
    if (firstOpen == -1 && firstBracket == -1) return -1;

    int baseEnd;
    if (firstBracket == -1 || (firstOpen != -1 && firstOpen < firstBracket)) {
        baseEnd = findMatchingClose(e, firstOpen);
    } else {
        baseEnd = findMatchingClose(e, firstBracket);
    }
    if (baseEnd == -1) return -1;

    // Skip whitespace between the base and the chain.
    int pos = baseEnd + 1;
    while (pos < e.length() && (e.at(pos) == ' ' || e.at(pos) == '\t')) pos++;
    if (pos >= e.length()) return -1;
    char c = e.at(pos);
    return (c == '.' || c == '[') ? pos : -1;
}

// Resolves a base expression to the live, writable slot that backs it (a scope
// variable, or a struct member / array element reached through '.key'/'[expr]'
// navigation) so a mutating member method called on it (arr.append(4),
// st.delete("x"), arr[1].push(9)) writes through to the caller's variable,
// matching the JIT path. Returns nullptr when the base is not an lvalue (a
// function call, a literal, a query column materialization, an unresolved
// path, ...); the caller then evaluates it by value and mutations stay
// confined to that snapshot.
static cfvariant *resolveWritableSlot(const string &baseExpr, string &out,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    string e = baseExpr.trimmed();
    if (e.isEmpty()) return nullptr;

    // The dotted prefix up to the first '[' must be a pure identifier/dot path
    // (a bare, scope-qualified or nested variable name) for lookupVarWritable
    // to resolve it to a live slot. Anything else (a function call, a literal)
    // is not an lvalue.
    int firstBracket = e.indexOf('[');
    string dotted = (firstBracket < 0) ? e : e.left(firstBracket).trimmed();
    if (dotted.isEmpty()) return nullptr;
    for (int i = 0; i < dotted.length(); i++) {
        char dc = dotted.at(i);
        if (!(isalnum(static_cast<unsigned char>(dc)) || dc == '_' || dc == '.')) {
            return nullptr;
        }
    }

    // Resolve the root first. A dotted base can cross component-valued
    // properties (for example currentAuthor.currentRole.preferences); looking
    // up the complete dotted spelling in the root scope loses the live CFC
    // slot and prevents the terminal member call from dispatching correctly.
    int firstDot = dotted.indexOf('.');
    string rootName = firstDot < 0 ? dotted : dotted.left(firstDot);
    cfvariant *cur = lookupVarWritable(rootName.constData(), cgi, server, cookie, application, session, url, form, variables);
    if (!cur) return nullptr;
    // Component instances are reference objects. Let the caller evaluate a
    // dotted component base by value; this preserves the instance identity
    // while avoiding treating the component's shared this-scope map as an
    // ordinary writable struct slot.
    if (cur->m_type == cfvariant::Component) return nullptr;

    // Walk any trailing '[expr]' / '.key' segments, mirroring applyMemberChain's
    // navigation but keeping live pointers so a later member method writes
    // through. A query-column reference is deliberately not followed: its cells
    // must be read through the owning query, not through the materialized copy.
    size_t pos = static_cast<size_t>(rootName.length());
    while (pos < (size_t)e.length()) {
        char c = e.at(pos);
        if (c == '.') {
            pos++;
            size_t idStart = pos;
            while (pos < (size_t)e.length()) {
                char mc = e.at(pos);
                if (isalnum(static_cast<unsigned char>(mc)) || mc == '_') pos++;
                else break;
            }
            string key = e.mid(static_cast<int>(idStart), static_cast<int>(pos - idStart));
            if (key.isEmpty() || !cur ||
                (cur->m_type != cfvariant::Struct && cur->m_type != cfvariant::Xml &&
                 cur->m_type != cfvariant::Component)) {
                return nullptr;
            }
            key.toUpper();
            auto it = cur->m_struct->find(key);
            if (it == cur->m_struct->end()) return nullptr;
            cur = &it->second;
        } else if (c == '[') {
            int close = findMatchingClose(e, static_cast<int>(pos));
            if (close == -1) return nullptr;
            string idxText = e.mid(static_cast<int>(pos) + 1, close - static_cast<int>(pos) - 1).trimmed();
            cfvariant idxVal = evaluateExpr(out, idxText, cgi, server, cookie, application, session, url, form, variables);
            if (cur->m_type == cfvariant::Array && cur->m_array && !cur->m_queryColOwner) {
                int idx = (idxVal.m_type == cfvariant::Number) ? idxVal.m_int : atoi(idxVal.toString().constData());
                if (idx < 1 || idx > (int)cur->m_array->size()) return nullptr;
                cur = &cur->m_array->at(idx - 1);
            } else if ((cur->m_type == cfvariant::Struct || cur->m_type == cfvariant::Xml) && cur->m_struct) {
                string key = idxVal.toString();
                key.toUpper();
                auto it = cur->m_struct->find(key);
                if (it == cur->m_struct->end()) return nullptr;
                cur = &it->second;
            } else {
                return nullptr;
            }
            pos = static_cast<size_t>(close) + 1;
        } else {
            return nullptr;
        }
    }
    return cur;
}

// Applies a chain of member accesses ('.key' / '[expr]') to a base value,
// left to right, returning the final value. Struct keys are matched
// case-insensitively (keys are stored upper-cased).
//
// `slot`, when non-null, is the live writable slot backing `base` (a scope
// variable or a member/element of one). A mutating member method (`arr.append(4)`,
// `st.delete("x")`, `arr.sort()`) then writes through to the slot, matching the
// JIT path. When slot is null, `base` is a snapshot (an array/struct literal, a
// function result, an unresolved path, ...) and mutations stay confined to it.
static cfvariant applyMemberChain(const cfvariant &base, cfvariant *slot,
    const string &chain,
    string &out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    cfvariant owned = base;    // snapshot used while no live slot exists
    cfvariant *cur = slot;     // live slot, or null -> operate on owned
    int pos = 0;
    while (pos < chain.length()) {
        char c = chain.at(pos);
        if (c == '.') {
            int end = pos + 1;
            while (end < chain.length()) {
                char cc = chain.at(end);
                if (cc == '.' || cc == '[' || cc == '(') break;
                end++;
            }
            string key = chain.mid(pos + 1, end - pos - 1).trimmed();
            if (key.isEmpty()) {
                throw webstrada::exception("Invalid member access in expression: " + chain);
            }
            // A `.method(args)` segment: dispatch the member method and
            // continue the chain on its result (e.g. `s.k[2].toUpperCase()`,
            // `arr.last().len()`).
            if (end < chain.length() && chain.at(end) == '(') {
                int closeParen = findMatchingClose(chain, end);
                if (closeParen == -1) {
                    throw webstrada::exception("Unterminated '(' in member access: " + chain);
                }
                FuncCall fc;
                parseFuncCall(key + "(" + chain.mid(end + 1, closeParen - end - 1) + ")", fc);
                std::vector<cfvariant> args;
                for (const auto &a : fc.args) {
                    args.push_back(evaluateExpr(out, a, cgi, server, cookie, application, session, url, form, variables));
                }
                std::vector<const cfvariant*> argPtrs;
                for (const auto &a : args) argPtrs.push_back(&a);
                // Dispatch on the live slot when one exists so a mutating method
                // writes through to the caller's variable; otherwise the snapshot.
                cfvariant *recv = cur ? cur : &owned;
                cfvariant result = invokeMemberMethod(*recv, key, argPtrs.data(), static_cast<int>(argPtrs.size()),
                    out, cgi, server, cookie, application, session, url, form, variables);
                owned = result;
                cur = nullptr;
                pos = closeParen + 1;
                continue;
            }
            key.toUpper();
            cfvariant *cv = cur ? cur : &owned;
            if (cv->m_type == cfvariant::Query && cv->m_query) {
                // Query pseudo-properties resolve via dot access.
                if (key.equals("COLUMNLIST")) {
                    cfvariant next(queryColumnList(cv));
                    owned = next;
                    cur = nullptr;
                    pos = end;
                    continue;
                }
                if (key.equals("RECORDCOUNT")) {
                    cfvariant next(queryRecordCount(cv));
                    owned = next;
                    cur = nullptr;
                    pos = end;
                    continue;
                }
                if (key.equals("CURRENTROW")) {
                    cfvariant next(cv->m_query->currentRow);
                    owned = next;
                    cur = nullptr;
                    pos = end;
                    continue;
                }
                int colIdx = cv->m_query->findColumn(key);
                if (colIdx >= 0) {
                    // Return the column as an Array carrying a live query-column
                    // reference so q.col[1] reads the current cell and copies
                    // behave like CF (see cfvariant.h m_queryCol* fields).
                    cfvariant next(cfvariant::Array);
                    for (auto &v : cv->m_query->columns[colIdx].values) next.insert(v);
                    next.m_queryColOwner = query_data_retain(cv->m_query);
                    next.m_queryColIndex = colIdx;
                    next.m_queryColFromBracket = false;
                    next.m_queryColWritable = true;
                    owned = next;
                    cur = nullptr;
                    pos = end;
                    continue;
                }
                throw webstrada::exception("Element '" + key + "' is undefined in Q.");
            }
            if (cv->m_type == cfvariant::Component && cv->m_component) {
                // Component member: this-scope data member or a method handle.
                if (cv->m_struct) {
                    auto it = cv->m_struct->find(key);
                    if (it != cv->m_struct->end()) {
                        if (cur) {
                            cur = &it->second;
                        } else {
                            cfvariant next = it->second;
                            owned = next;
                        }
                        pos = end;
                        continue;
                    }
                }
                if (cfvariant *h = componentMemberAccess(cv, key)) {
                    owned = *h;
                    cur = nullptr;
                    pos = end;
                    continue;
                }
                throw webstrada::exception("Element '" + key + "' is undefined in O.");
            }
            if (cv->m_type != cfvariant::Struct && cv->m_type != cfvariant::Xml) {
                throw webstrada::exception("Cannot access member '" + key + "' of a non-struct value");
            }
            auto it = cv->m_struct->find(key);
            if (it == cv->m_struct->end()) {
                throw webstrada::exception("Element '" + key + "' is undefined");
            }
            if (cur) {
                // Navigate to the live member slot so a later method mutates it.
                cur = &it->second;
            } else {
                // Snapshot before overwriting owned: operator= destroys owned's
                // own container first, which would otherwise invalidate it.
                cfvariant next = it->second;
                owned = next;
            }
            pos = end;
        } else if (c == '[') {
            int closePos = findMatchingClose(chain, pos);
            if (closePos == -1) {
                throw webstrada::exception("Unterminated '[' in member access: " + chain);
            }
            string idxExpr = chain.mid(pos + 1, closePos - pos - 1).trimmed();
            cfvariant idxVal = evaluateExpr(out, idxExpr, cgi, server, cookie, application, session, url, form, variables);
            cfvariant *cv = cur ? cur : &owned;
            if (cv->m_type == cfvariant::Query && cv->m_query) {
                string key = idxVal.toString();
                // Bracket access resolves columns before pseudo-properties
                // (verified against CF 2021).
                int colIdx = cv->m_query->findColumn(key);
                if (colIdx >= 0) {
                    cfvariant next(cfvariant::Array);
                    for (auto &v : cv->m_query->columns[colIdx].values) next.insert(v);
                    next.m_queryColOwner = query_data_retain(cv->m_query);
                    next.m_queryColIndex = colIdx;
                    next.m_queryColFromBracket = true;
                    next.m_queryColWritable = true;
                    owned = next;
                    cur = nullptr;
                } else {
                    string upper = key;
                    upper.toUpper();
                    if (upper.equals("COLUMNLIST")) {
                        cfvariant next(queryColumnList(cv));
                        owned = next;
                        cur = nullptr;
                    } else if (upper.equals("RECORDCOUNT")) {
                        cfvariant next(queryRecordCount(cv));
                        owned = next;
                        cur = nullptr;
                    } else if (upper.equals("CURRENTROW")) {
                        cfvariant next(cv->m_query->currentRow);
                        owned = next;
                        cur = nullptr;
                    } else {
                        throw webstrada::exception("Element '" + key + "' is undefined in Q.");
                    }
                }
                pos = closePos + 1;
            } else if (cv->m_type == cfvariant::Array && cv->m_array) {
                int idx = (idxVal.m_type == cfvariant::Number) ? idxVal.m_int : atoi(idxVal.toString().constData());
                // A live query-column reference reads through to the query's
                // cells; a row past the last one reads as empty (CF 2021).
                if (cv->m_queryColOwner && cv->m_queryColIndex >= 0) {
                    QueryData *qd = cv->m_queryColOwner;
                    int colIdx = cv->m_queryColIndex;
                    if (colIdx >= 0 && colIdx < (int)qd->columns.size()) {
                        QueryColumn &col = qd->columns[colIdx];
                        if (idx >= 1 && idx <= (int)col.values.size()) {
                            owned = col.values[idx - 1];
                        } else {
                            owned = cfvariant(cfvariant::Null);
                        }
                        cur = nullptr;
                        pos = closePos + 1;
                        continue;
                    }
                }
                if (idx < 1 || idx > (int)cv->m_array->size()) {
                    throw webstrada::exception("Array index out of bounds: " + chain);
                }
                if (cur) {
                    // Navigate to the live element slot so a later method mutates it.
                    cur = &cur->m_array->at(idx - 1);
                } else {
                    cfvariant next = cv->m_array->at(idx - 1);
                    owned = next;
                }
            } else if (cv->m_type == cfvariant::String) {
                // CF indexes strings by position; a re-copied column reference
                // degrades to such a scalar (its first cell).
                int idx = (idxVal.m_type == cfvariant::Number) ? idxVal.m_int : atoi(idxVal.toString().constData());
                if (idx < 1 || idx > (int)cv->m_str->length()) {
                    throw webstrada::exception(string("Cannot access array element at position ") + string::number(idx) + ".");
                }
                cfvariant next(cfvariant::String);
                next.m_str->clear();
                next.m_str->append(cv->m_str->at(idx - 1));
                owned = next;
                cur = nullptr;
            } else if ((cv->m_type == cfvariant::Struct || cv->m_type == cfvariant::Xml) && cv->m_struct) {
                string key = idxVal.toString();
                key.toUpper();
                auto it = cv->m_struct->find(key);
                if (it == cv->m_struct->end()) {
                    throw webstrada::exception("Element '" + key + "' is undefined");
                }
                if (cur) {
                    cur = &it->second;
                } else {
                    cfvariant next = it->second;
                    owned = next;
                }
            } else if (cv->m_type == cfvariant::Component && cv->m_component) {
                string key = idxVal.toString();
                key.toUpper();
                if (cv->m_struct) {
                    auto it = cv->m_struct->find(key);
                    if (it != cv->m_struct->end()) {
                        if (cur) {
                            cur = &it->second;
                        } else {
                            cfvariant next = it->second;
                            owned = next;
                        }
                        pos = closePos + 1;
                        continue;
                    }
                }
                if (cfvariant *h = componentMemberAccess(cv, key)) {
                    owned = *h;
                    cur = nullptr;
                    pos = closePos + 1;
                    continue;
                }
                throw webstrada::exception("Element '" + key + "' is undefined in O.");
            } else {
                throw webstrada::exception("Cannot index into a value that is not an array or struct");
            }
            pos = closePos + 1;
        } else {
            break;
        }
    }
    return cur ? *cur : owned;
}

// True when s is a "simple name reference": a bare identifier, an optionally
// parenthesized identifier, or a dotted chain of identifiers (a, (a), s.x,
// s.x.y). Used to match CF's rejection of indexing an array literal with such
// a reference ([1,2][a] and [1,2][s.x] are rejected; [1,2][a + 0],
// [1,2][Abs(-1)], [1,2][s["x"]] and [1,2][2] are accepted).
static bool isSimpleNameReference(const string &s)
{
    string t = s.trimmed();
    if (t.length() >= 2 && t.at(0) == '(' && t.at(t.length() - 1) == ')') {
        t = t.mid(1, t.length() - 2).trimmed();
    }
    if (t.isEmpty()) return false;
    // CFML boolean literals are constants, not name references: [1,2][true]
    // is accepted by CF, so they must not be rejected here.
    if (t.compareCaseInsensitive("true") == 0 || t.compareCaseInsensitive("false") == 0 ||
        t.compareCaseInsensitive("yes") == 0 || t.compareCaseInsensitive("no") == 0) {
        return false;
    }
    int i = 0;
    if (!isalpha(static_cast<unsigned char>(t.at(i))) && t.at(i) != '_') return false;
    i++;
    while (i < (int)t.length()) {
        char c = t.at(i);
        if (isalnum(static_cast<unsigned char>(c)) || c == '_') {
            i++;
            continue;
        }
        if (c == '.' && i + 1 < (int)t.length()) {
            i++;
            if (!isalpha(static_cast<unsigned char>(t.at(i))) && t.at(i) != '_') return false;
            i++;
            continue;
        }
        return false;
    }
    return true;
}

// Applies a trailing member-access chain ('.key' / '[expr]') to an array or
// struct literal base, consuming the chain from s starting at pos. Returns the
// resulting value with pos advanced past the chain.
static cfvariant appendLiteralChain(const cfvariant &base, const string &s, size_t &pos,
    string &out, void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    size_t chainStart = pos;
    size_t end = pos;
    while (end < (size_t)s.length()) {
        char cc = s.at(end);
        if (cc == '.') {
            end++;
            while (end < (size_t)s.length()) {
                char mc = s.at(end);
                if (isalnum(static_cast<unsigned char>(mc)) || mc == '_') end++;
                else break;
            }
            // A `.method(args)` segment: include the argument list so the call
            // is dispatched through invokeMemberMethod (e.g. [1,2].toList()).
            if (end < (size_t)s.length() && s.at(end) == '(') {
                int cl = findMatchingClose(s, (int)end);
                if (cl == -1) break;
                end = (size_t)cl + 1;
            }
        } else if (cc == '[') {
            int cl = findMatchingClose(s, (int)end);
            if (cl == -1) break;
            end = (size_t)cl + 1;
        } else {
            break;
        }
    }
    if (end > chainStart) {
        string chain = s.mid(chainStart, end - chainStart);
        // CF rejects indexing an array literal with a simple name reference,
        // matching the JIT path (see the ArrayIndex handling in
        // parseTokensToAST).
        if (base.m_type == cfvariant::Array && !chain.isEmpty() && chain.at(0) == '[') {
            int cl = findMatchingClose(chain, 0);
            if (cl != -1 && isSimpleNameReference(chain.mid(1, cl - 1))) {
                throw webstrada::exception("Cannot index an array literal with a variable");
            }
        }
        pos = end;
        return applyMemberChain(base, nullptr, chain, out, cgi, server, cookie, application, session, url, form, variables);
    }
    pos = chainStart;
    return base;
}

// Finds the index of the '#' that closes the interpolation opened at start
// (s[start] == '#'), skipping over quoted strings and nested parentheses so a
// '#' inside a string argument (e.g. Replace("a#b", ...)) is not mistaken for
// the terminator. Returns -1 when no closing '#' exists.
static int findSharpExprEnd(const string &s, int start)
{
    int parenDepth = 0;
    bool inDouble = false;
    bool inSingle = false;
    for (int i = start + 1; i < s.length(); i++) {
        char c = s.at(i);
        if (c == '"' && !inSingle) {
            inDouble = !inDouble;
        } else if (c == '\'' && !inDouble) {
            inSingle = !inSingle;
        } else if (!inDouble && !inSingle) {
            if (c == '(') {
                parenDepth++;
            } else if (c == ')') {
                parenDepth--;
            } else if (c == '#' && parenDepth == 0) {
                return i;
            }
        }
    }
    return -1;
}

// Renders a value the way cfoutput does (String/Number/Float/Boolean/DateTime).
// unsupportedName is used to reproduce cfoutputexpr's error message; when null
// (string interpolation) a generic message is thrown instead.
static void appendExprValueAsString(string &target, cfvariant &val,
                                    const char *unsupportedName)
{
    switch (val.m_type) {
    case cfvariant::String:
        if (val.m_str) target.append(*val.m_str);
        break;
    case cfvariant::Function:
        // A method handle for a built-in function (coldfusion.runtime.CFPageMethod@…).
        if (val.m_str) target.append(*val.m_str);
        break;
    case cfvariant::Number:
        target.append(string::number(val.m_int));
        break;
    case cfvariant::Long:
        target.append(string::number(val.m_long));
        break;
    case cfvariant::Float: {
        // CF preserves the literal text of float literals (8.0 -> "8.0");
        // computed doubles render with 12 significant digits and a normalized
        // 3-digit exponent (2^53 -> 9.00719925474E+015).
        if (val.m_literalText) {
            target.append(*val.m_literalText);
        } else {
            std::string s = formatCfdumpFloat(val.m_double);
            target.append(s.c_str());
        }
        break;
    }
    case cfvariant::Boolean:
        if (val.m_boolLiteral) {
            target.append(val.m_bool ? "true" : "false");
        } else {
            target.append(val.m_bool ? "YES" : "NO");
        }
        break;
    case cfvariant::Binary:
        // A cfhttp ByteArrayOutputStream (getasbinary="no" + non-text MIME)
        // outputs as the bytes decoded with the default charset (UTF-8, invalid
        // -> U+FFFD); a plain byte[] cannot be output.
        if (val.m_isByteArrayOutputStream && val.m_binary) {
            target.append(val.toString());
            break;
        }
        if (unsupportedName) {
            webstrada::string msg("Cannot output variable '");
            msg.append(unsupportedName);
            msg.append("' of unsupported type");
            throw webstrada::exception(msg);
        }
        throw webstrada::exception("Cannot output value of unsupported type");
    case cfvariant::DateTime:
        target.append(val.toString());
        break;
    case cfvariant::Null:
        break;
    case cfvariant::Array:
        // A live query-column reference stringifies as its first cell's value
        // (CF 2021: x = q["a"]; #x# prints the first row's cell). A plain
        // array cannot be stringified.
        if (val.m_queryColOwner && val.m_queryColIndex >= 0) {
            cfvariant first = queryColumnFirstCell(&val);
            appendExprValueAsString(target, first, unsupportedName);
            break;
        }
        if (unsupportedName) {
            webstrada::string msg("Cannot output variable '");
            msg.append(unsupportedName);
            msg.append("' of unsupported type");
            throw webstrada::exception(msg);
        }
        throw webstrada::exception("Cannot output value of unsupported type");
    default:
        if (unsupportedName) {
            webstrada::string msg("Cannot output variable '");
            msg.append(unsupportedName);
            msg.append("' of unsupported type");
            throw webstrada::exception(msg);
        }
        throw webstrada::exception("Cannot output value of unsupported type");
    }
}

// Evaluates a quoted string literal (e contains the outer quotes): collapses
// escaped quotes ("" -> " / '' -> '), converts ## to # and interpolates
// #expr# sub-expressions (e.g. "a #x# b" with x=5 -> "a 5 b").
static cfvariant evaluateStringLiteral(string &out, const string &e, char quote,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    string lit = e.mid(1, e.length() - 2);
    string result;
    for (int i = 0; i < lit.length(); i++) {
        char c = lit.at(i);
        if (c == quote && i + 1 < lit.length() && lit.at(i + 1) == quote) {
            // Escaped quote inside a same-quoted literal: "" -> ", '' -> '
            result.append(quote);
            i++;
        } else if (c == '#' && i + 1 < lit.length() && lit.at(i + 1) == '#') {
            // Escaped hash: ## -> #
            result.append('#');
            i++;
        } else if (c == '#') {
            int end = findSharpExprEnd(lit, i);
            if (end == -1) {
                result.append(c);
            } else {
                string inner = lit.mid(i + 1, end - i - 1).trimmed();
                cfvariant val = evaluateExpr(out, inner, cgi, server, cookie, application, session, url, form, variables);
                appendExprValueAsString(result, val, nullptr);
                i = end;
            }
        } else {
            result.append(c);
        }
    }
    return cfvariant(result);
}

// Returns true when s is a valid CFML numeric literal: digits, an optional
// '.', optional exponent, and an optional leading sign (the tokenizer's Number
// grammar). Unlike strtod/strtoll this rejects C99 forms like inf/infinity/nan
// and hex floats, so a variable named inf/nan/infinity is not swallowed as a
// numeric literal in #...# expressions.
static bool isCfmNumericLiteral(const string &s)
{
    if (s.isEmpty()) return false;
    int i = 0;
    if (s.at(0) == '+' || s.at(0) == '-') i++;
    bool digitsBefore = false;
    bool digitsAfter = false;
    while (i < s.length() && isdigit(static_cast<unsigned char>(s.at(i)))) {
        i++;
        digitsBefore = true;
    }
    if (i < s.length() && s.at(i) == '.') {
        i++;
        while (i < s.length() && isdigit(static_cast<unsigned char>(s.at(i)))) {
            i++;
            digitsAfter = true;
        }
    }
    if (!digitsBefore && !digitsAfter) return false;
    if (i < s.length() && (s.at(i) == 'e' || s.at(i) == 'E')) {
        i++;
        if (i < s.length() && (s.at(i) == '+' || s.at(i) == '-')) i++;
        int expStart = i;
        while (i < s.length() && isdigit(static_cast<unsigned char>(s.at(i)))) i++;
        if (i == expStart) return false;
    }
    return i == s.length();
}

// ---- Full binary/unary operator support for #...# template expressions ----
// Mirrors the precedence table in llvm_codegen.cpp getOpPrecedence and the CFML
// precedence verified in CFML_Precedence.md. Word operators are matched
// case-insensitively as whole words; symbolic operators are matched by their
// characters. The parser is a classic precedence-climbing descent:
//   sharpParseExpr   - binary operators at/above a minimum precedence
//   sharpParseUnary  - unary NOT/!/-,+ (which bind at their own precedence)
//   sharpParsePrimary- a single atom: (expr), "string", number, identifier
//                      chains (func(...).a[1]...)

static int sharpBinOpPrec(const string &op)
{
    if (op.equals("IMP")) return 1;
    if (op.equals("?")) return 1; // ternary ? : (right-assoc; handled in sharpParseExpr)
    if (op.equals("EQV")) return 2;
    if (op.equals("XOR")) return 3;
    if (op.equals("OR") || op.equals("||")) return 4;
    if (op.equals("AND") || op.equals("&&")) return 5;
    if (op.equals("EQ") || op.equals("NEQ") || op.equals("LT") || op.equals("LTE") || op.equals("LE") || op.equals("GT") || op.equals("GTE") || op.equals("GE") ||
        op.equals("IS") || op.equals("EQUAL") || op.equals("CONTAINS") ||
        op.equals("DOES NOT CONTAIN") || op.equals("IS NOT") || op.equals("NOT EQUAL") ||
        op.equals("==") || op.equals("!=")) return 8;
    if (op.equals("&")) return 9;
    if (op.equals("+") || op.equals("-")) return 10;
    if (op.equals("*") || op.equals("/") || op.equals("\\") || op.equals("MOD") || op.equals("%")) return 11;
    if (op.equals("^")) return 12;
    return 0;
}

static int sharpUnaryOpPrec(const string &op)
{
    // Unary -/+ bind tighter than '^' (CF: -2^2 == (-2)^2 == 4); NOT/! bind
    // looser than comparisons (CF: NOT 0 GT 3 == NOT(0 GT 3) == true).
    if (op.equals("-") || op.equals("+")) return 13;
    if (op.equals("NOT") || op.equals("!")) return 6;
    return 0;
}

// Case-insensitive whole-word match of `word` starting at s[begin]. The word
// must be bounded by non-identifier characters on both sides.
static bool sharpWordStartsAt(const string &s, size_t begin, const char *word)
{
    size_t wlen = strlen(word);
    if (begin + wlen > (size_t)s.length()) return false;
    for (size_t k = 0; k < wlen; k++) {
        if (tolower(static_cast<unsigned char>(s.at(begin + k))) !=
            tolower(static_cast<unsigned char>(word[k]))) return false;
    }
    size_t end = begin + wlen;
    if (end < (size_t)s.length()) {
        char c = s.at(end);
        if (isalnum(static_cast<unsigned char>(c)) || c == '_') return false;
    }
    if (begin > 0) {
        char p = s.at(begin - 1);
        if (isalnum(static_cast<unsigned char>(p)) || p == '_') return false;
    }
    return true;
}

// Tries to match a binary operator at pos (caller has skipped whitespace). On
// success advances pos and returns the operator text in opOut.
static bool matchSharpBinaryOp(const string &s, size_t &pos, string &opOut)
{
    if (pos >= (size_t)s.length()) return false;

    // Word operators, longest first so "IS NOT" wins over "IS".
    static const char *wordOps[] = {
        "DOES NOT CONTAIN", "GREATER THAN OR EQUAL TO", "LESS THAN OR EQUAL TO",
        "GREATER THAN", "LESS THAN", "IS NOT", "NOT EQUAL",
        "MOD", "EQV", "IMP", "XOR", "AND", "OR", "EQ", "NEQ", "IS",
        "EQUAL", "CONTAINS", "GTE", "LTE", "GT", "LT", "GE", "LE",
        nullptr
    };
    for (int w = 0; wordOps[w]; w++) {
        if (sharpWordStartsAt(s, pos, wordOps[w])) {
            opOut = wordOps[w];
            pos += strlen(wordOps[w]);
            return true;
        }
    }

    char c = s.at(pos);
    if (pos + 1 < (size_t)s.length()) {
        char n = s.at(pos + 1);
        if ((c == '=' && n == '=') || (c == '!' && n == '=') ||
            (c == '&' && n == '&') || (c == '|' && n == '|')) {
            opOut = s.mid(pos, 2);
            pos += 2;
            return true;
        }
    }
    if (c == '?') {
        opOut = "?";
        pos += 1;
        return true;
    }
    if (c == '^' || c == '*' || c == '/' || c == '\\' || c == '%' || c == '&' ||
        c == '+' || c == '-') {
        opOut = string(1, c);
        pos += 1;
        return true;
    }
    return false;
}

// Tries to match a unary operator at pos (caller has skipped whitespace).
static bool matchSharpUnaryOp(const string &s, size_t &pos, string &opOut)
{
    if (pos >= (size_t)s.length()) return false;
    if (sharpWordStartsAt(s, pos, "NOT")) {
        opOut = "NOT";
        pos += 3;
        return true;
    }
    char c = s.at(pos);
    if (c == '!' || c == '-' || c == '+') {
        opOut = string(1, c);
        pos += 1;
        return true;
    }
    return false;
}

// Applies a binary operator to two already-evaluated operands. Routes through
// the same cfvariant_* runtime used by the JIT path so both stay consistent.
static cfvariant applySharpBinaryOp(string &out, const string &op, cfvariant &lv, cfvariant &rv,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    (void)out;
    if (op.equals("+")) return *cfml::cfvariant_add(&lv, &rv);
    if (op.equals("-")) return *cfml::cfvariant_sub(&lv, &rv);
    if (op.equals("*")) return *cfml::cfvariant_mul(&lv, &rv);
    if (op.equals("/")) return *cfml::cfvariant_div(&lv, &rv);
    if (op.equals("\\")) return *cfml::cfvariant_idiv(&lv, &rv);
    if (op.equals("^")) return *cfml::cfvariant_pow(&lv, &rv);
    if (op.equals("MOD") || op.equals("%")) return *cfml::cfvariant_mod(&lv, &rv);
    if (op.equals("&")) return *cfml::cfvariant_concat(&lv, &rv);
    if (op.equals("AND") || op.equals("&&")) return *cfml::cfvariant_and(&lv, &rv);
    if (op.equals("OR") || op.equals("||")) return *cfml::cfvariant_or(&lv, &rv);
    if (op.equals("XOR")) return *cfml::cfvariant_xor(&lv, &rv);
    // EQV / IMP / comparisons (word and symbolic forms) via compareVariants.
    return *cfml::cfvariant_compare(&lv, &rv, op.constData());
}

static cfvariant sharpParsePrimary(string &out, const string &s, size_t &pos,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    if (pos >= (size_t)s.length()) throw webstrada::exception("Unexpected end of expression");

    size_t start = pos;
    char c = s.at(pos);

    // Parenthesized sub-expression: evaluate the inner text as a fresh
    // full expression so operators inside bind before the outer ones.
    if (c == '(') {
        int close = findMatchingClose(s, pos);
        if (close == -1) throw webstrada::exception("Unterminated '(' in expression");
        string inner = s.mid(pos + 1, close - pos - 1).trimmed();
        cfvariant val = evaluateExpr(out, inner, cgi, server, cookie, application, session, url, form, variables);
        pos = close + 1;
        return val;
    }

    // Array literal: [elem1, elem2, ...]. Elements may themselves be
    // array/struct literals or arbitrary expressions.
    if (c == '[') {
        int close = findMatchingClose(s, pos);
        if (close == -1) throw webstrada::exception("Unterminated '[' in array literal");
        string inner = s.mid(pos + 1, close - pos - 1);
        cfvariant arr(cfvariant::Array);
        auto elems = splitTopLevel(inner, ',');
        for (const auto &elemStr : elems) {
            if (elemStr.isEmpty()) continue;
            cfvariant elem = evaluateExpr(out, elemStr, cgi, server, cookie, application, session, url, form, variables);
            arr.insert(elem);
        }
        pos = close + 1;
        return appendLiteralChain(arr, s, pos, out, cgi, server, cookie, application, session, url, form, variables);
    }

    // Struct literal: {key:value, ...} — ColdFusion accepts both the
    // `key:value` and `key=value` forms.
    if (c == '{') {
        int close = findMatchingClose(s, pos);
        if (close == -1) throw webstrada::exception("Unterminated '{' in struct literal");
        string inner = s.mid(pos + 1, close - pos - 1);
        cfvariant st(cfvariant::Struct);
        auto pairs = splitTopLevel(inner, ',');
        for (const auto &pairStr : pairs) {
            if (pairStr.isEmpty()) continue;
            int sep = findTopLevelPairSep(pairStr);
            if (sep == -1) {
                throw webstrada::exception("Struct literal entry is missing a ':' or '=' separator: " + pairStr);
            }
            string key = pairStr.left(sep).trimmed();
            if ((key.length() >= 2 && key.first() == '"' && key.at(key.length() - 1) == '"') ||
                (key.length() >= 2 && key.first() == '\'' && key.at(key.length() - 1) == '\'')) {
                // Quoted keys keep their casing ({"aB":1} → "aB").
                key = key.mid(1, key.length() - 2);
            } else {
                // CF uppercases unquoted struct-literal keys ({a:1} → "A").
                key.toUpper();
            }
            string valStr = pairStr.mid(sep + 1, pairStr.length() - sep - 1).trimmed();
            cfvariant val = evaluateExpr(out, valStr, cgi, server, cookie, application, session, url, form, variables);
            st.structSet(key, val);
        }
        pos = close + 1;
        return appendLiteralChain(st, s, pos, out, cgi, server, cookie, application, session, url, form, variables);
    }

    // Quoted string literal: scan to the closing (un-escaped) quote.
    if (c == '"' || c == '\'') {
        char q = c;
        pos++;
        while (pos < (size_t)s.length()) {
            char cc = s.at(pos);
            if (cc == q) {
                if (pos + 1 < (size_t)s.length() && s.at(pos + 1) == q) {
                    pos += 2;
                    continue;
                }
                pos++;
                break;
            }
            pos++;
        }
        string lit = s.mid(start, pos - start);
        return evaluateExpr(out, lit, cgi, server, cookie, application, session, url, form, variables, false);
    }

    // Numeric literal: digits, optional '.', optional exponent. A leading '.'
    // followed by a digit (.5) is a CFML number, not a member access.
    if (isdigit(static_cast<unsigned char>(c)) ||
        (c == '.' && pos + 1 < (size_t)s.length() && isdigit(static_cast<unsigned char>(s.at(pos + 1))))) {
        while (pos < (size_t)s.length()) {
            char cc = s.at(pos);
            if (isdigit(static_cast<unsigned char>(cc)) || cc == '.') {
                pos++;
                continue;
            }
            if (cc == 'e' || cc == 'E') {
                size_t save = pos;
                pos++;
                if (pos < (size_t)s.length() && (s.at(pos) == '+' || s.at(pos) == '-')) pos++;
                if (pos < (size_t)s.length() && isdigit(static_cast<unsigned char>(s.at(pos)))) {
                    while (pos < (size_t)s.length() && isdigit(static_cast<unsigned char>(s.at(pos)))) pos++;
                } else {
                    pos = save;
                }
                continue;
            }
            break;
        }
        string num = s.mid(start, pos - start);
        return evaluateExpr(out, num, cgi, server, cookie, application, session, url, form, variables, false);
    }

    // Identifier: variable / function call / member access / array index chain.
    if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
        while (pos < (size_t)s.length()) {
            char cc = s.at(pos);
            if (isalnum(static_cast<unsigned char>(cc)) || cc == '_') pos++;
            else break;
        }
        for (;;) {
            if (pos >= (size_t)s.length()) break;
            char cc = s.at(pos);
            if (cc == '(') {
                int close = findMatchingClose(s, pos);
                if (close == -1) throw webstrada::exception("Unterminated '(' in expression");
                pos = close + 1;
            } else if (cc == '[') {
                int close = findMatchingClose(s, pos);
                if (close == -1) throw webstrada::exception("Unterminated '[' in expression");
                pos = close + 1;
            } else if (cc == '.') {
                pos++;
                while (pos < (size_t)s.length()) {
                    char mc = s.at(pos);
                    if (isalnum(static_cast<unsigned char>(mc)) || mc == '_') pos++;
                    else break;
                }
            } else {
                break;
            }
        }
        string atom = s.mid(start, pos - start);
        return evaluateExpr(out, atom, cgi, server, cookie, application, session, url, form, variables, false);
    }

    throw webstrada::exception("Unexpected character in expression");
}

static cfvariant sharpParseUnary(string &out, const string &s, size_t &pos, int minPrec,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    (void)minPrec;
    while (pos < (size_t)s.length() && (s.at(pos) == ' ' || s.at(pos) == '\t')) pos++;
    string op;
    if (matchSharpUnaryOp(s, pos, op)) {
        int prec = sharpUnaryOpPrec(op);
        cfvariant operand = sharpParseExpr(out, s, pos, prec, cgi, server, cookie, application, session, url, form, variables);
        if (op.equals("NOT") || op.equals("!")) return *cfml::cfvariant_not(&operand);
        if (op.equals("-")) return *cfml::cfvariant_neg(&operand);
        return operand; // unary '+'
    }
    return sharpParsePrimary(out, s, pos, cgi, server, cookie, application, session, url, form, variables);
}

// Advances `pos` past a full expression operand (the same token extent that
// sharpParseExpr would consume at the given minimum precedence) WITHOUT
// evaluating anything. Used for AND/OR short-circuit: when the left operand
// decides the result (false AND ..., true OR ...) the right operand must be
// skipped, not evaluated — CF never touches it (verified: `true OR
// undefinedvar` -> true, `lastIndex EQ 0 OR arr[lastIndex]...` with
// lastIndex=0 never reads arr[0], was Queue.cfc:30). Mirror the grammar:
// primary (parens/brackets/strings/numbers/identifiers + chains), unary
// NOT/!/+/- and the binary/ternary loop.
static void sharpSkipExpr(const string &s, size_t &pos, int minPrec)
{
    auto skipWhitespace = [&]() {
        while (pos < (size_t)s.length() && (s.at(pos) == ' ' || s.at(pos) == '\t')) pos++;
    };
    auto skipPrimary = [&]() {
        if (pos >= (size_t)s.length()) throw webstrada::exception("Unexpected end of expression");
        char c = s.at(pos);
        if (c == '(' || c == '[' || c == '{') {
            int close = findMatchingClose(s, pos);
            if (close == -1) throw webstrada::exception("Unterminated '" + string(1, c) + "' in expression");
            pos = close + 1;
            // A literal may carry a trailing member/index chain
            // (`[1,2,3][5]`, `{a:1}.b`, `(x).y`): consume it like
            // appendLiteralChain does so the skipped extent matches what the
            // evaluator would have consumed.
            for (;;) {
                if (pos >= (size_t)s.length()) break;
                char cc = s.at(pos);
                if (cc == '[') {
                    int cl = findMatchingClose(s, pos);
                    if (cl == -1) break;
                    pos = cl + 1;
                } else if (cc == '.') {
                    pos++;
                    while (pos < (size_t)s.length()) {
                        char mc = s.at(pos);
                        if (isalnum(static_cast<unsigned char>(mc)) || mc == '_') pos++;
                        else break;
                    }
                    if (pos < (size_t)s.length() && s.at(pos) == '(') {
                        int cl = findMatchingClose(s, pos);
                        if (cl == -1) break;
                        pos = cl + 1;
                    }
                } else {
                    break;
                }
            }
            return;
        }
        if (c == '"' || c == '\'') {
            char q = c;
            pos++;
            while (pos < (size_t)s.length()) {
                char cc = s.at(pos);
                if (cc == q) {
                    if (pos + 1 < (size_t)s.length() && s.at(pos + 1) == q) { pos += 2; continue; }
                    pos++;
                    break;
                }
                pos++;
            }
            return;
        }
        if (isdigit(static_cast<unsigned char>(c)) ||
            (c == '.' && pos + 1 < (size_t)s.length() && isdigit(static_cast<unsigned char>(s.at(pos + 1))))) {
            while (pos < (size_t)s.length()) {
                char cc = s.at(pos);
                if (isdigit(static_cast<unsigned char>(cc)) || cc == '.') { pos++; continue; }
                if (cc == 'e' || cc == 'E') {
                    size_t save = pos;
                    pos++;
                    if (pos < (size_t)s.length() && (s.at(pos) == '+' || s.at(pos) == '-')) pos++;
                    if (pos < (size_t)s.length() && isdigit(static_cast<unsigned char>(s.at(pos)))) {
                        while (pos < (size_t)s.length() && isdigit(static_cast<unsigned char>(s.at(pos)))) pos++;
                    } else {
                        pos = save;
                    }
                    continue;
                }
                break;
            }
            return;
        }
        if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
            while (pos < (size_t)s.length()) {
                char cc = s.at(pos);
                if (isalnum(static_cast<unsigned char>(cc)) || cc == '_') pos++;
                else break;
            }
            for (;;) {
                if (pos >= (size_t)s.length()) break;
                char cc = s.at(pos);
                if (cc == '(' || cc == '[') {
                    int close = findMatchingClose(s, pos);
                    if (close == -1) throw webstrada::exception("Unterminated '" + string(1, cc) + "' in expression");
                    pos = close + 1;
                } else if (cc == '.') {
                    pos++;
                    while (pos < (size_t)s.length()) {
                        char mc = s.at(pos);
                        if (isalnum(static_cast<unsigned char>(mc)) || mc == '_') pos++;
                        else break;
                    }
                } else {
                    break;
                }
            }
            return;
        }
        pos++;
    };

    // Unary prefix.
    skipWhitespace();
    {
        size_t save = pos;
        string op;
        if (matchSharpUnaryOp(s, pos, op)) {
            int prec = sharpUnaryOpPrec(op);
            sharpSkipExpr(s, pos, prec);
        } else {
            pos = save;
            skipPrimary();
        }
    }

    for (;;) {
        size_t save = pos;
        skipWhitespace();
        string op;
        if (!matchSharpBinaryOp(s, pos, op)) {
            if (pos < (size_t)s.length() && (s.at(pos) == '<' || s.at(pos) == '>')) {
                throw webstrada::exception("Illegal symbolic comparison operator inside #...# (use the word forms GT, LT, GTE or LTE)");
            }
            pos = save;
            return;
        }
        int prec = sharpBinOpPrec(op);
        if (prec < minPrec) {
            pos = save;
            return;
        }
        if (op.equals("?")) {
            size_t thenPos = pos;
            size_t scan = thenPos;
            int depth = 0;
            size_t colonPos = (size_t)-1;
            for (;;) {
                while (scan < (size_t)s.length() && (s.at(scan) == ' ' || s.at(scan) == '\t')) scan++;
                if (scan >= (size_t)s.length()) break;
                char cc = s.at(scan);
                if (cc == '(') { depth++; scan++; continue; }
                if (cc == ')') {
                    if (depth == 0) break;
                    depth--; scan++; continue;
                }
                if (cc == '?' && depth == 0) { depth++; scan++; continue; }
                if (cc == ':' && depth == 0) { colonPos = scan; break; }
                if (cc == '"' || cc == '\'') {
                    char q = cc;
                    scan++;
                    while (scan < (size_t)s.length() && s.at(scan) != q) scan++;
                    scan++;
                    continue;
                }
                scan++;
            }
            if (colonPos == (size_t)-1) {
                throw webstrada::exception("Invalid ternary expression: missing ':'");
            }
            size_t p = thenPos;
            sharpSkipExpr(s, p, 0);
            p = colonPos + 1;
            sharpSkipExpr(s, p, 0);
            pos = p;
            continue;
        }
        sharpSkipExpr(s, pos, prec + 1);
    }
}

static cfvariant sharpParseExpr(string &out, const string &s, size_t &pos, int minPrec,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables)
{
    cfvariant lhs = sharpParseUnary(out, s, pos, minPrec, cgi, server, cookie, application, session, url, form, variables);
    for (;;) {
        size_t save = pos;
        while (pos < (size_t)s.length() && (s.at(pos) == ' ' || s.at(pos) == '\t')) pos++;
        string op;
        if (!matchSharpBinaryOp(s, pos, op)) {
            // CF rejects the symbolic < > <= >= comparison operators inside
            // #...# (the tag parser consumes < and > as tag delimiters), so the
            // engine must not evaluate them leniently; only the word forms
            // (GT/LT/GTE/LTE) and ==/!= are valid there.
            if (pos < (size_t)s.length() && (s.at(pos) == '<' || s.at(pos) == '>')) {
                throw webstrada::exception("Illegal symbolic comparison operator inside #...# (use the word forms GT, LT, GTE or LTE)");
            }
            pos = save;
            break;
        }
        int prec = sharpBinOpPrec(op);
        if (prec < minPrec) {
            pos = save;
            break;
        }
        if (op.equals("?")) {
            // Ternary: lhs ? then : else (right-associative, lowest precedence).
            // The "then" expression runs until a top-level ':'; "else" is the
            // remainder, parsed at min precedence so nested ternaries work.
            cfvariant cond = lhs;
            size_t thenPos = pos;
            // Scan for the matching ':' that closes this ternary, tracking
            // nested '?'/'(' depth.
            size_t scan = thenPos;
            int depth = 0;
            size_t colonPos = (size_t)-1;
            for (;;) {
                while (scan < (size_t)s.length() && (s.at(scan) == ' ' || s.at(scan) == '\t')) scan++;
                if (scan >= (size_t)s.length()) break;
                char cc = s.at(scan);
                if (cc == '(') { depth++; scan++; continue; }
                if (cc == ')') {
                    if (depth == 0) break;
                    depth--; scan++; continue;
                }
                if (cc == '?' && depth == 0) { depth++; scan++; continue; }
                if (cc == ':' && depth == 0) { colonPos = scan; break; }
                // Skip a quoted string literal.
                if (cc == '"' || cc == '\'') {
                    char q = cc;
                    scan++;
                    while (scan < (size_t)s.length() && s.at(scan) != q) scan++;
                    scan++;
                    continue;
                }
                scan++;
            }
            if (colonPos == (size_t)-1) {
                throw webstrada::exception("Invalid ternary expression: missing ':'");
            }
            cfvariant thenVal = sharpParseExpr(out, s, thenPos, 0, cgi, server, cookie, application, session, url, form, variables);
            pos = colonPos + 1;
            cfvariant elseVal = sharpParseExpr(out, s, pos, 0, cgi, server, cookie, application, session, url, form, variables);
            lhs = *cf_ternary_select(&cond, &thenVal, &elseVal);
            // The else consumed the rest; continue the outer loop (nothing left
            // or a higher-precedence operator follows at top level).
            continue;
        }
        if (op.equals("AND") || op.equals("&&") || op.equals("OR") || op.equals("||")) {
            // CF short-circuits AND/OR: the right operand is evaluated only
            // when needed (verified: `true OR undefinedvar` -> true, `lastIndex
            // EQ 0 OR arr[lastIndex]...` with lastIndex=0 never touches arr[0]).
            // When the left side decides the result, skip the rhs tokens
            // without evaluating them; otherwise evaluate normally.
            bool isAnd = op.equals("AND") || op.equals("&&");
            int lhsTruthy = cfml::isTruthy(lhs);
            bool needRhs = isAnd ? (lhsTruthy != 0) : (lhsTruthy == 0);
            if (needRhs) {
                cfvariant rhs = sharpParseExpr(out, s, pos, prec + 1, cgi, server, cookie, application, session, url, form, variables);
                lhs = applySharpBinaryOp(out, op, lhs, rhs, cgi, server, cookie, application, session, url, form, variables);
            } else {
                // Short-circuit: the left operand decides the result. AND with a
                // falsy lhs returns lhs (cfvariant_and), OR with a truthy lhs
                // returns lhs (cfvariant_or). Skip the rhs tokens un-evaluated.
                sharpSkipExpr(s, pos, prec + 1);
            }
            continue;
        }
        cfvariant rhs = sharpParseExpr(out, s, pos, prec + 1, cgi, server, cookie, application, session, url, form, variables);
        lhs = applySharpBinaryOp(out, op, lhs, rhs, cgi, server, cookie, application, session, url, form, variables);
    }
    return lhs;
}

// True when `s` is a bare identifier (letters/digits/underscore), i.e. the
// chain-base form whose undefined resolution CF reports differently for a dot
// member access ("Element KEY is undefined in BASE.") than for bracket access.
static bool isSimpleIdentifierName(const string &s)
{
    if (s.isEmpty()) return false;
    for (int i = 0; i < s.length(); i++) {
        char c = s.at(i);
        if (!(isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
    }
    return true;
}

cfvariant evaluateExpr(string &out, const string &expr,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables,
    bool parseBinary)
{
    string e = expr.trimmed();
    if (e.isEmpty()) return cfvariant(cfvariant::Null);

    // Full binary/unary operator evaluation with CFML precedence. The parser
    // consumes the whole expression when it is a valid operator expression;
    // otherwise (e.g. a bare variable, literal, function call, member chain, or
    // malformed input) it falls through to the single-atom handling below.
    if (parseBinary) {
        size_t pos = 0;
        cfvariant val = sharpParseExpr(out, e, pos, 0, cgi, server, cookie, application, session, url, form, variables);
        while (pos < (size_t)e.length() && (e.at(pos) == ' ' || e.at(pos) == '\t')) pos++;
        if (pos == (size_t)e.length()) return val;
    }

    // 3. Split off a trailing member-access chain ('.key' / '[expr]') from the
    //    base expression, e.g. d.ITEMS[1].ID or func().a[1].b. The base is
    //    evaluated recursively, then each chain segment is applied in order.
    {
        int chainStart = findChainStart(e);
        if (chainStart > 0) {
            string baseExpr = e.left(chainStart).trimmed();
            // CF rejects indexing/member access directly on a parenthesized
            // expression ((arr)[1], (s).a) — verified against CF 2021. It does
            // allow chains on function calls (DeserializeJSON(...).a.b) and on
            // bare variables/array/struct literals.
            if (!baseExpr.isEmpty() && baseExpr.first() == '(') {
                throw webstrada::exception("Cannot index or access a member of a parenthesized expression");
            }
            string chain = e.mid(chainStart, e.length() - chainStart).trimmed();
            // Resolve the base to its live writable slot (when it is an lvalue)
            // so a mutating member method in the chain writes through to the
            // caller's variable, matching the JIT path. When there is no slot,
            // evaluate the base by value and mutations stay on the snapshot.
            cfvariant *slot = resolveWritableSlot(baseExpr, out, cgi, server, cookie, application, session, url, form, variables);
            cfvariant base;
            if (slot) {
                base = cfvariant(cfvariant::Null);
            } else {
                // A `.key` on an undefined chain base reports CF's ELEMENT
                // message ("Element KEY is undefined in BASE.") rather than the
                // variable message used by bracket access — verified against CF
                // 2025 (was BUGS.md "chain-base lookups"). Only a bare
                // identifier base with a leading '.' takes this path.
                if (!chain.isEmpty() && chain.first() == '.' &&
                    isSimpleIdentifierName(baseExpr)) {
                    int dotEnd = 1;
                    while (dotEnd < chain.length()) {
                        char dc = chain.at(dotEnd);
                        if (dc == '.' || dc == '[' || dc == '(') break;
                        dotEnd++;
                    }
                    string key = chain.mid(1, dotEnd - 1).trimmed();
                    string upKey = key, upBase = baseExpr;
                    upKey.toUpper();
                    upBase.toUpper();
                    webstrada::string msg("Element ");
                    msg.append(upKey);
                    msg.append(" is undefined in ");
                    msg.append(upBase);
                    msg.append(".");
                    throw webstrada::exception(msg);
                }
                int baseDot = baseExpr.indexOf('.');
                if (baseDot > 0) {
                    string rootName = baseExpr.left(baseDot).trimmed();
                    cfvariant *rootSlot = lookupVarWritable(rootName.constData(), cgi, server, cookie, application, session, url, form, variables);
                    if (rootSlot) {
                        string baseChain = baseExpr.mid(baseDot, baseExpr.length() - baseDot).trimmed();
                        base = applyMemberChain(*rootSlot, nullptr, baseChain, out,
                                                cgi, server, cookie, application,
                                                session, url, form, variables);
                    } else {
                        base = evaluateExpr(out, baseExpr, cgi, server, cookie, application, session, url, form, variables);
                    }
                } else {
                    base = evaluateExpr(out, baseExpr, cgi, server, cookie, application, session, url, form, variables);
                }
            }
            return applyMemberChain(base, slot, chain, out, cgi, server, cookie, application, session, url, form, variables);
        }
    }

    // 4. Check if it is a function call
    FuncCall call;
    if (parseFuncCall(e, call)) {
        string fname = call.name;
        fname.toUpper();

        // Compiler-extension functions (the reserved `__` prefix, like C's
        // `__` identifiers): dispatch directly to the native implementation
        // before any member/UDF/variable resolution. An unregistered `__name`
        // call is a compile-time-style error, matching the JIT path.
        if (fname.length() > 2 && fname.at(0) == '_' && fname.at(1) == '_') {
            std::vector<const cfvariant*> evalArgs;
            for (const auto &a : call.args) {
                cfvariant ev = evaluateExpr(out, a, cgi, server, cookie, application, session, url, form, variables);
                auto *tmp = new cfvariant(ev);
                cf_register_temp(tmp);
                evalArgs.push_back(tmp);
            }
            cfvariant *res = cfml::cf_extension_call(fname.constData(),
                                                     evalArgs.empty() ? nullptr : evalArgs.data(),
                                                     static_cast<int>(evalArgs.size()));
            if (!res) {
                throw webstrada::exception(webstrada::string("Unknown compiler extension function ") + fname + ".");
            }
            return tempReturn(res);
        }

        // Member-method call written as `base.method(args)` where the base has
        // no trailing index chain (e.g. `s.k.toUpperCase()`, `getFoo().bar()`).
        // Split the callee at the last dot, evaluate the base expression, and
        // dispatch the method with the evaluated arguments.
        int memberDot = call.name.lastIndexOf('.');
        if (memberDot > 0) {
            string baseStr = call.name.left(memberDot).trimmed();
            string methodName = call.name.mid(memberDot + 1, call.name.length() - memberDot - 1).trimmed();
            // Resolve the base to its live writable slot (when it is an lvalue)
            // so a mutating method (`arr.append(4)`) writes through to the
            // caller's variable, matching the JIT path.
            cfvariant *slot = resolveWritableSlot(baseStr, out, cgi, server, cookie, application, session, url, form, variables);
            cfvariant base;
            if (slot) {
                base = cfvariant(cfvariant::Null);
            } else {
                base = evaluateExpr(out, baseStr, cgi, server, cookie, application, session, url, form, variables);
            }
            // Named arguments on a member call (base.method(name=value)) build
            // the marker struct passed as args[0]; the runtime member dispatch
            // reorders against the method's parameter names.
            bool anyNamed = false;
            for (const auto &a : call.args) {
                string nm, vl;
                if (splitNamedArg(a, nm, vl)) { anyNamed = true; break; }
            }
            std::vector<cfvariant> args;
            std::vector<const cfvariant*> argPtrs;
            if (anyNamed) {
                auto *namedStruct = new cfvariant(cfvariant::Struct);
                cf_register_temp(namedStruct);
                for (const auto &a : call.args) {
                    string nm, vl;
                    if (splitNamedArg(a, nm, vl)) {
                        cfvariant val = evaluateExpr(out, vl, cgi, server, cookie, application, session, url, form, variables);
                        // Keep the original casing (see the UDF path above).
                        namedStruct->structSet(nm, val);
                    } else {
                        cfvariant ev = evaluateExpr(out, a, cgi, server, cookie, application, session, url, form, variables);
                        args.push_back(ev);
                    }
                }
                auto *marker = cfml::cf_named_args_marker(namedStruct);
                cf_register_temp(marker);
                argPtrs.push_back(marker);
                for (const auto &a : args) argPtrs.push_back(&a);
            } else {
                for (const auto &a : call.args) {
                    args.push_back(evaluateExpr(out, a, cgi, server, cookie, application, session, url, form, variables));
                }
                for (const auto &a : args) argPtrs.push_back(&a);
            }
            if (slot) {
                return invokeMemberMethod(*slot, methodName, argPtrs.data(), static_cast<int>(argPtrs.size()),
                    out, cgi, server, cookie, application, session, url, form, variables);
            }
            return invokeMemberMethod(base, methodName, argPtrs.data(), static_cast<int>(argPtrs.size()),
                out, cgi, server, cookie, application, session, url, form, variables);
        }

        // UDF/closure dispatch: the callee name is resolved as a variable (CF
        // stores functions in the variables scope). A callable Function value
        // is invoked directly; a non-function variable cannot be called.
        cfvariant *udfVal = lookupVarWritable(call.name.constData(), cgi, server, cookie, application, session, url, form, variables);
        if (udfVal && udfVal->m_type == cfvariant::Function && udfVal->m_udf && udfVal->m_udf->fn) {
            std::vector<const cfvariant*> evalArgs;
            // Named arguments: build the marker struct (args[0]) so cf_udf_invoke
            // binds by parameter name. Positional args follow in order.
            bool anyNamed = false;
            for (const auto &a : call.args) {
                string nm, vl;
                if (splitNamedArg(a, nm, vl)) { anyNamed = true; break; }
            }
            if (anyNamed) {
                auto *namedStruct = new cfvariant(cfvariant::Struct);
                cf_register_temp(namedStruct);
                for (const auto &a : call.args) {
                    string nm, vl;
                    if (splitNamedArg(a, nm, vl)) {
                        cfvariant val = evaluateExpr(out, vl, cgi, server, cookie, application, session, url, form, variables);
                        // Keep the original casing: cf_named_args_reorder matches
                        // declared params case-insensitively, and extra named args
                        // that match no parameter surface in the arguments scope
                        // under the casing they were written with (CF behavior).
                        namedStruct->structSet(nm, val);
                    } else {
                        cfvariant ev = evaluateExpr(out, a, cgi, server, cookie, application, session, url, form, variables);
                        auto *tmp = new cfvariant(ev);
                        cf_register_temp(tmp);
                        evalArgs.push_back(tmp);
                    }
                }
                cfvariant *marker = cfml::cf_named_args_marker(namedStruct);
                cf_register_temp(marker);
                evalArgs.insert(evalArgs.begin(), marker);
            } else {
                for (const auto &a : call.args) {
                    cfvariant ev = evaluateExpr(out, a, cgi, server, cookie, application, session, url, form, variables);
                    auto *tmp = new cfvariant(ev);
                    cf_register_temp(tmp);
                    evalArgs.push_back(tmp);
                }
            }
            cfvariant *res = cfml::cf_udf_invoke(udfVal, evalArgs.data(), static_cast<int>(evalArgs.size()),
                                           out, cgi, server, cookie, application, session, url, form, variables);
            return *res;
        }
        if (udfVal && udfVal->m_type != cfvariant::Function) {
            // CF: a built-in function wins over a non-function variable with
            // the same name; only a non-builtin name in a non-function variable
            // raises "Entity has incorrect type for being called as a function."
            if (!isKnownFunctionName(fname)) {
                throw webstrada::exception("Entity has incorrect type for being called as a function.");
            }
        }

        if (fname.equals("EVALUATE")) {
            if (call.args.empty()) throw webstrada::exception("The Evaluate function takes 1 or more parameters.");
            cfvariant result;
            for (const auto &a : call.args) {
                cfvariant av = evaluateExpr(out, a, cgi, server, cookie, application, session, url, form, variables);
                string exprStr = av.toString();
                result = evaluateExpr(out, exprStr, cgi, server, cookie, application, session, url, form, variables, true);
            }
            return result;
        }

        if (fname.equals("ARRAYNEW")) {
            if (call.args.size() > 1 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("ArrayNew requires 0 or 1 arguments");
            }
            return tempReturn(cfml::cf_arraynew());
        }

        if (fname.equals("QUERYNEW")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("QueryNew requires 1 to 3 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1;
            if (call.args.size() >= 2) {
                arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            cfvariant arg2;
            if (call.args.size() == 3) {
                arg2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_querynew(&arg0, call.args.size() >= 2 ? &arg1 : nullptr, call.args.size() == 3 ? &arg2 : nullptr));
        }

        if (fname.equals("QUERYADDROW")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("QueryAddRow requires 1 or 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var || var->m_type != cfvariant::Query) throw webstrada::exception("QueryAddRow: First argument must be a query variable");
            cfvariant rowsArg;
            cfvariant *rowsPtr = nullptr;
            if (call.args.size() == 2) {
                rowsArg = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
                rowsPtr = &rowsArg;
            }
            return tempReturn(cfml::cf_queryaddrow(var, rowsPtr));
        }

        if (fname.equals("QUERYSETCELL")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception("QuerySetCell requires 3 or 4 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var || var->m_type != cfvariant::Query) throw webstrada::exception("QuerySetCell: First argument must be a query variable");
            cfvariant col = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant rowArg;
            cfvariant *rowPtr = nullptr;
            if (call.args.size() == 4) {
                rowArg = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
                rowPtr = &rowArg;
            }
            return tempReturn(cfml::cf_querysetcell(var, &col, &val, rowPtr));
        }

        if (fname.equals("QUERYADDCOLUMN")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception("QueryAddColumn requires 3 or 4 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var || var->m_type != cfvariant::Query) throw webstrada::exception("QueryAddColumn: First argument must be a query variable");
            cfvariant col = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *datatype = nullptr;
            cfvariant *arrPtr = &arg2;
            if (call.args.size() == 4) {
                datatype = &arg2;
                arrPtr = new cfvariant(evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables));
                cf_register_temp(arrPtr);
            }
            return tempReturn(cfml::cf_queryaddcolumn(var, &col, datatype, arrPtr));
        }

        if (fname.equals("QUERYGETROW")) {
            if (call.args.size() != 2) throw webstrada::exception("QueryGetRow requires exactly 2 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_querygetrow(&arg0, &arg1));
        }

        if (fname.equals("QUERYKEYEXISTS")) {
            if (call.args.size() != 2) throw webstrada::exception("QueryKeyExists requires exactly 2 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_querykeyexists(&arg0, &arg1));
        }

        if (fname.equals("QUERYCONVERTFORGRID")) {
            if (call.args.size() != 3) throw webstrada::exception("QueryConvertForGrid requires exactly 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_queryconvertforgrid(&a0, &a1, &a2));
        }
        if (fname.equals("QUERYEACH")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("QueryEach requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_queryeach(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                       out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("QUERYEXECUTE")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("QueryExecute requires 1 to 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1, a2;
            if (call.args.size() >= 2) a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_queryexecute(&a0, call.args.size() >= 2 ? &a1 : nullptr, call.args.size() == 3 ? &a2 : nullptr,
                                          cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("PRECISIONEVALUATE")) {
            if (call.args.size() != 1) throw webstrada::exception("PrecisionEvaluate requires exactly 1 argument");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_precisionevaluate(&a0, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("QUERYFILTER")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("QueryFilter requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_queryfilter(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                         out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("QUERYGETRESULT")) {
            if (call.args.size() != 1) throw webstrada::exception("QueryGetResult requires exactly 1 argument");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_querygetresult(&a0));
        }
        if (fname.equals("QUERYMAP")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("QueryMap requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_querymap(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                      out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("QUERYREDUCE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("QueryReduce requires 2 or 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_queryreduce(&a0, &a1, call.args.size() == 3 ? &a2 : nullptr,
                                         out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("QUOTEDVALUELIST")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("QuotedValueList requires 1 or 2 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1;
            if (call.args.size() == 2) {
                a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_quotedvaluelist(&a0, call.args.size() == 2 ? &a1 : nullptr));
        }

        if (fname.equals("VALUELIST")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ValueList requires 1 or 2 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1;
            if (call.args.size() == 2) {
                arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_valuelist(&arg0, call.args.size() == 2 ? &arg1 : nullptr));
        }

        if (fname.equals("ARRAYEACH")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("ArrayEach requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayeach(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                       out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("ARRAYFILTER")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("ArrayFilter requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayfilter(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                         out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("ARRAYREDUCE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ArrayReduce requires 2 or 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayreduce(&a0, &a1, call.args.size() == 3 ? &a2 : nullptr,
                                         out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("LISTEACH")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("ListEach requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listeach(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                      out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("LISTFILTER")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("ListFilter requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listfilter(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                        out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("LISTGETDUPLICATES")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ListGetDuplicates requires 1 or 2 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1;
            if (call.args.size() == 2) a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listgetduplicates(&a0, call.args.size() == 2 ? &a1 : nullptr));
        }
        if (fname.equals("LISTMAP")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("ListMap requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listmap(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                     out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("LISTQUALIFY")) {
            if (call.args.size() < 2 || call.args.size() > 5) throw webstrada::exception("ListQualify requires 2 to 5 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3, a4;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 5) a4 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listqualify(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr,
                                         call.args.size() >= 4 ? &a3 : nullptr, call.args.size() == 5 ? &a4 : nullptr));
        }
        if (fname.equals("LISTREDUCE")) {
            if (call.args.size() < 2 || call.args.size() > 5) throw webstrada::exception("ListReduce requires 2 to 5 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3, a4;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 5) a4 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listreduce(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr,
                                        call.args.size() >= 4 ? &a3 : nullptr, call.args.size() == 5 ? &a4 : nullptr,
                                        out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("LISTREMOVEDUPLICATES")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("ListRemoveDuplicates requires 1 to 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1, a2;
            if (call.args.size() >= 2) a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listremoveduplicates(&a0, call.args.size() >= 2 ? &a1 : nullptr, call.args.size() == 3 ? &a2 : nullptr));
        }
        if (fname.equals("LISTSORT")) {
            if (call.args.size() < 2 || call.args.size() > 6) throw webstrada::exception("ListSort requires 2 to 6 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3, a4, a5;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 5) a4 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 6) a5 = evaluateExpr(out, call.args[5], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listsort(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr,
                                      call.args.size() >= 4 ? &a3 : nullptr, call.args.size() >= 5 ? &a4 : nullptr,
                                      call.args.size() == 6 ? &a5 : nullptr,
                                      out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("LISTVALUECOUNT")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListValueCount requires 2 or 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listvaluecount(&a0, &a1, call.args.size() == 3 ? &a2 : nullptr));
        }
        if (fname.equals("LISTVALUECOUNTNOCASE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListValueCountNoCase requires 2 or 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_listvaluecountnocase(&a0, &a1, call.args.size() == 3 ? &a2 : nullptr));
        }
        if (fname.equals("STRUCTAPPEND")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("StructAppend requires 2 or 3 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var || (var->m_type != cfvariant::Struct && var->m_type != cfvariant::Component)) throw webstrada::exception("StructAppend: First argument must be a struct variable");
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structappend(var, &a1, call.args.size() == 3 ? &a2 : nullptr));
        }
        if (fname.equals("STRUCTCOPY")) {
            if (call.args.size() != 1) throw webstrada::exception("StructCopy requires exactly 1 argument");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structcopy(&a0));
        }
        if (fname.equals("STRUCTEACH")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("StructEach requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structeach(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                        out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("STRUCTFILTER")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("StructFilter requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structfilter(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                          out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("STRUCTFINDKEY")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("StructFindKey requires 2 or 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structfindkey(&a0, &a1, call.args.size() == 3 ? &a2 : nullptr));
        }
        if (fname.equals("STRUCTFINDVALUE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("StructFindValue requires 2 or 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structfindvalue(&a0, &a1, call.args.size() == 3 ? &a2 : nullptr));
        }
        if (fname.equals("STRUCTGET")) {
            if (call.args.size() != 1) throw webstrada::exception("StructGet requires exactly 1 argument");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structget(&a0, variables));
        }
        if (fname.equals("STRUCTGETMETADATA")) {
            if (call.args.size() != 1) throw webstrada::exception("StructGetMetadata requires exactly 1 argument");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structgetmetadata(&a0));
        }
        if (fname.equals("STRUCTMAP")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("StructMap requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structmap(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr, call.args.size() == 4 ? &a3 : nullptr,
                                       out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("STRUCTREDUCE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("StructReduce requires 2 or 3 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() == 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structreduce(&a0, &a1, call.args.size() == 3 ? &a2 : nullptr,
                                          out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("STRUCTSETMETADATA")) {
            if (call.args.size() != 2) throw webstrada::exception("StructSetMetadata requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var || var->m_type != cfvariant::Struct) throw webstrada::exception("StructSetMetadata: First argument must be a struct variable");
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structsetmetadata(var, &a1));
        }
        if (fname.equals("STRUCTSORT")) {
            if (call.args.size() < 2 || call.args.size() > 5) throw webstrada::exception("StructSort requires 2 to 5 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3, a4;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 5) a4 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structsort(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr,
                                        call.args.size() >= 4 ? &a3 : nullptr, call.args.size() == 5 ? &a4 : nullptr,
                                        out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("STRUCTTOSORTED")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("StructToSorted requires 2 to 4 arguments");
            cfvariant a0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3;
            if (call.args.size() >= 3) a2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) a3 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structtosorted(&a0, &a1, call.args.size() >= 3 ? &a2 : nullptr,
                                            call.args.size() == 4 ? &a3 : nullptr,
                                            out, cgi, server, cookie, application, session, url, form, variables));
        }

        if (fname.equals("ARRAYLEN")) {
            if (call.args.size() != 1) throw webstrada::exception("ArrayLen requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraylen(var ? var : &arg));
        }

        if (fname.equals("ARRAYFIRST") || fname.equals("ARRAYLAST") || fname.equals("ARRAYPOP") || fname.equals("ARRAYSHIFT")) {
            if (call.args.size() != 1) throw webstrada::exception("ArrayFirst/ArrayLast/ArrayPop/ArrayShift require exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayFirst/ArrayLast/ArrayPop/ArrayShift: First argument must be an array variable");
            if (fname.equals("ARRAYFIRST")) return tempReturn(cfml::cf_arrayfirst(var));
            if (fname.equals("ARRAYLAST")) return tempReturn(cfml::cf_arraylast(var));
            if (fname.equals("ARRAYPOP")) return tempReturn(cfml::cf_arraypop(var));
            return tempReturn(cfml::cf_arrayshift(var));
        }

        if (fname.equals("ARRAYAPPEND")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ArrayAppend requires 2 or 3 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayAppend: First argument must be an array variable");
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant merge;
            if (call.args.size() == 3) merge = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayappend(var, &val, call.args.size() == 3 ? &merge : nullptr));
        }

        if (fname.equals("ARRAYPREPEND")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayPrepend requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayPrepend: First argument must be an array variable");
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayprepend(var, &val));
        }

        if (fname.equals("ARRAYISEMPTY")) {
            if (call.args.size() != 1) throw webstrada::exception("ArrayIsEmpty requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayisempty(var ? var : &arg));
        }

        if (fname.equals("ARRAYCLEAR")) {
            if (call.args.size() != 1) throw webstrada::exception("ArrayClear requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayClear: First argument must be an array variable");
            return tempReturn(cfml::cf_arrayclear(var));
        }

        if (fname.equals("ARRAYDELETEAT")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayDeleteAt requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayDeleteAt: First argument must be an array variable");
            cfvariant idx = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraydeleteat(var, &idx));
        }

        if (fname.equals("ARRAYINSERTAT")) {
            if (call.args.size() != 3) throw webstrada::exception("ArrayInsertAt requires exactly 3 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayInsertAt: First argument must be an array variable");
            cfvariant idx = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayinsertat(var, &idx, &val));
        }

        if (fname.equals("ARRAYRESIZE")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayResize requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayResize: First argument must be an array variable");
            cfvariant sz = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayresize(var, &sz));
        }

        if (fname.equals("ARRAYSET")) {
            if (call.args.size() != 4) throw webstrada::exception("ArraySet requires exactly 4 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArraySet: First argument must be an array variable");
            cfvariant start = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant end = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayset(var, &start, &end, &val));
        }

        if (fname.equals("ARRAYTOLIST")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ArrayToList requires 1 or 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 2) {
                delim = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_arraytolist(var ? var : &arg, call.args.size() == 2 ? &delim : nullptr));
        }

        if (fname.equals("ARRAYAVG")) {
            if (call.args.size() != 1) throw webstrada::exception("ArrayAvg requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayavg(var ? var : &arg));
        }

        if (fname.equals("ARRAYSUM")) {
            if (call.args.size() != 1) throw webstrada::exception("ArraySum requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraysum(var ? var : &arg));
        }

        if (fname.equals("ARRAYMIN")) {
            if (call.args.size() != 1) throw webstrada::exception("ArrayMin requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraymin(var ? var : &arg));
        }

        if (fname.equals("ARRAYMAX")) {
            if (call.args.size() != 1) throw webstrada::exception("ArrayMax requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraymax(var ? var : &arg));
        }

        if (fname.equals("ARRAYCONTAINS")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayContains requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraycontains(var ? var : &arg, &val));
        }

        if (fname.equals("ARRAYCONTAINSNOCASE")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayContainsNoCase requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraycontainsnocase(var ? var : &arg, &val));
        }

        if (fname.equals("ARRAYFIND")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayFind requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayfind(var ? var : &arg, &val));
        }

        if (fname.equals("ARRAYFINDNOCASE")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayFindNoCase requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayfindnocase(var ? var : &arg, &val));
        }

        if (fname.equals("ARRAYFINDALL")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayFindAll requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayfindall(var ? var : &arg, &val));
        }

        if (fname.equals("ARRAYFINDALLNOCASE")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayFindAllNoCase requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayfindallnocase(var ? var : &arg, &val));
        }

        if (fname.equals("ARRAYDELETE")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayDelete requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayDelete: First argument must be an array variable");
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraydelete(var, &val));
        }

        if (fname.equals("ARRAYDELETENOCASE")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayDeleteNoCase requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArrayDeleteNoCase: First argument must be an array variable");
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arraydeletenocase(var, &val));
        }

        if (fname.equals("ARRAYSLICE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ArraySlice requires 2 or 3 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant offset = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant length;
            if (call.args.size() == 3) {
                length = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_arrayslice(var ? var : &arg, &offset, call.args.size() == 3 ? &length : nullptr));
        }

        if (fname.equals("ARRAYSWAP")) {
            if (call.args.size() != 3) throw webstrada::exception("ArraySwap requires exactly 3 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("ArraySwap: First argument must be an array variable");
            cfvariant idx1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant idx2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayswap(var, &idx1, &idx2));
        }

        if (fname.equals("ISARRAY")) {
            if (call.args.size() != 1) throw webstrada::exception("IsArray requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isarray(var ? var : &arg));
        }

        if (fname.equals("ISQUERY")) {
            if (call.args.size() != 1) throw webstrada::exception("IsQuery requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isquery(var ? var : &arg));
        }

        if (fname.equals("STRUCTVALUEARRAY")) {
            if (call.args.size() != 1) throw webstrada::exception("StructValueArray requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structvaluearray(var ? var : &arg));
        }

        if (fname.equals("ARRAYISDEFINED")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayIsDefined requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant idx = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_arrayisdefined(var ? var : &arg, &idx));
        }

        if (fname.equals("ARRAYSETMETADATA")) {
            // Leave metadata inline or map as return true
            cfvariant res(cfvariant::Boolean);
            res.m_bool = true;
            return res;
        }

        if (fname.equals("ARRAYMAP")) {
            if (call.args.size() != 2) throw webstrada::exception("ArrayMap requires exactly 2 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg;
            if (!var) arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            const cfvariant *arrVal = var ? var : &arg;
            if (arrVal->m_type != cfvariant::Array) throw webstrada::exception("ArrayMap: First argument must be an array");
            cfvariant callback = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant result;
            result.set_type(cfvariant::Array);
            for (size_t i = 0; i < arrVal->m_array->size(); i++) {
                std::vector<cfvariant> cbArgs = { arrVal->m_array->at(i) };
                cfvariant mapped = callCallback(out, callback, cbArgs, cgi, server, cookie, application, session, url, form, variables);
                result.insert(mapped);
            }
            return result;
        }
        if (fname.equals("ARRAYEACH") || fname.equals("ARRAYFILTER") || fname.equals("ARRAYREDUCE") || fname.equals("ARRAYSORT")) {
            // Keep callback-heavy logic inline/delegated for now
        }

        if (fname.equals("LISTTOARRAY")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ListToArray requires 1 or 2 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 2) {
                delim = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listtoarray(&list, call.args.size() == 2 ? &delim : nullptr));
        }

        if (fname.equals("FILEEXISTS")) {
            if (call.args.size() != 1) throw webstrada::exception("FileExists requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_fileexists(&path));
        }

        if (fname.equals("DIRECTORYEXISTS")) {
            if (call.args.size() != 1) throw webstrada::exception("DirectoryExists requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_directoryexists(&path));
        }

        if (fname.equals("FILEREAD")) {
            if (call.args.size() != 1) throw webstrada::exception("FileRead requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_fileread(&path));
        }

        if (fname.equals("FILEWRITE")) {
            if (call.args.size() != 2) throw webstrada::exception("FileWrite requires exactly 2 arguments");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant content = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filewrite(&path, &content));
        }

        if (fname.equals("FILEDELETE")) {
            if (call.args.size() != 1) throw webstrada::exception("FileDelete requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filedelete(&path));
        }

        if (fname.equals("FILECOPY")) {
            if (call.args.size() != 2) throw webstrada::exception("FileCopy requires exactly 2 arguments");
            cfvariant src = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dest = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filecopy(&src, &dest));
        }

        if (fname.equals("FILEMOVE")) {
            if (call.args.size() != 2) throw webstrada::exception("FileMove requires exactly 2 arguments");
            cfvariant src = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dest = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filemove(&src, &dest));
        }

        if (fname.equals("DIRECTORYCREATE")) {
            if (call.args.size() != 1) throw webstrada::exception("DirectoryCreate requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_directorycreate(&path));
        }

        if (fname.equals("DIRECTORYDELETE")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("DirectoryDelete requires 1 or 2 arguments");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant recurse;
            if (call.args.size() == 2) {
                recurse = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_directorydelete(&path, call.args.size() == 2 ? &recurse : nullptr));
        }

        if (fname.equals("GETFILEINFO")) {
            if (call.args.size() != 1) throw webstrada::exception("GetFileInfo requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_getfileinfo(&path));
        }

        if (fname.equals("FILEOPEN")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("FileOpen requires 2 or 3 arguments");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant mode = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant charset;
            if (call.args.size() == 3) {
                charset = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            auto *ret = cfml::cf_fileopen(&path, &mode, call.args.size() == 3 ? &charset : nullptr);
            cf_register_temp(ret);
            cfvariant result;
            result.m_type = cfvariant::File;
            result.m_fd = ret->m_fd;
            ret->m_type = cfvariant::NotSet;
            return result;
        }

        if (fname.equals("FILECLOSE")) {
            if (call.args.size() != 1) throw webstrada::exception("FileClose requires exactly 1 argument");
            cfvariant fileObj = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_fileclose(&fileObj));
        }

        if (fname.equals("FILEREADLINE")) {
            if (call.args.size() != 1) throw webstrada::exception("FileReadLine requires exactly 1 argument");
            cfvariant fileObj = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filereadline(&fileObj));
        }

        if (fname.equals("FILEWRITELINE")) {
            if (call.args.size() != 2) throw webstrada::exception("FileWriteLine requires exactly 2 arguments");
            cfvariant fileObj = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant content = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filewriteline(&fileObj, &content));
        }

        if (fname.equals("FILESEEK")) {
            if (call.args.size() != 2) throw webstrada::exception("FileSeek requires exactly 2 arguments");
            cfvariant fileObj = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant position = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_fileseek(&fileObj, &position));
        }

        if (fname.equals("FILESKIPBYTES")) {
            if (call.args.size() != 2) throw webstrada::exception("FileSkipBytes requires exactly 2 arguments");
            cfvariant fileObj = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant count = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_fileskipbytes(&fileObj, &count));
        }

        if (fname.equals("FILEREADBINARY")) {
            if (call.args.size() != 1) throw webstrada::exception("FileReadBinary requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filereadbinary(&path));
        }

        if (fname.equals("BINARYDECODE")) {
            if (call.args.size() != 2) throw webstrada::exception("BinaryDecode requires exactly 2 arguments");
            cfvariant str = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant enc = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_binarydecode(&str, &enc));
        }

        if (fname.equals("FILEGETMIMETYPE")) {
            if (call.args.size() != 1) throw webstrada::exception("FileGetMimeType requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filegetmimetype(&path));
        }

        if (fname.equals("FILESETACCESSMODE")) {
            if (call.args.size() != 2) throw webstrada::exception("FileSetAccessMode requires exactly 2 arguments");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant mode = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filesetaccessmode(&path, &mode));
        }

        if (fname.equals("FILESETATTRIBUTE")) {
            if (call.args.size() != 2) throw webstrada::exception("FileSetAttribute requires exactly 2 arguments");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant attr = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filesetattribute(&path, &attr));
        }

        if (fname.equals("FILESETLASTMODIFIED")) {
            if (call.args.size() != 2) throw webstrada::exception("FileSetLastModified requires exactly 2 arguments");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant date = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_filesetlastmodified(&path, &date));
        }

        if (fname.equals("FILEUPLOAD")) {
            if (call.args.size() < 1 || call.args.size() > 5) throw webstrada::exception("FileUpload requires 1 to 5 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3, a4, a5;
            if (call.args.size() >= 2) a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 5) a5 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_fileupload(&a1,
                call.args.size() >= 2 ? &a2 : nullptr,
                call.args.size() >= 3 ? &a3 : nullptr,
                call.args.size() >= 4 ? &a4 : nullptr,
                call.args.size() >= 5 ? &a5 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("FILEUPLOADALL")) {
            if (call.args.size() < 1 || call.args.size() > 7) throw webstrada::exception("FileUploadAll requires 1 to 7 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3, a4, a5, a6, a7;
            if (call.args.size() >= 2) a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 5) a5 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 6) a6 = evaluateExpr(out, call.args[5], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 7) a7 = evaluateExpr(out, call.args[6], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_fileuploadall(static_cast<cfvariant*>(variables), &a1,
                call.args.size() >= 2 ? &a2 : nullptr,
                call.args.size() >= 3 ? &a3 : nullptr,
                call.args.size() >= 4 ? &a4 : nullptr,
                call.args.size() >= 5 ? &a5 : nullptr,
                call.args.size() >= 6 ? &a6 : nullptr,
                call.args.size() >= 7 ? &a7 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("HASH")) {
            if (call.args.size() < 1 || call.args.size() > 4) throw webstrada::exception("Hash requires 1 to 4 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2, a3, a4;
            if (call.args.size() >= 2) a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_hash(&a1,
                call.args.size() >= 2 ? &a2 : nullptr,
                call.args.size() >= 3 ? &a3 : nullptr,
                call.args.size() >= 4 ? &a4 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("HMAC")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("HMac requires 2 to 4 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3, a4;
            if (call.args.size() >= 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_hmac(&a1, &a2,
                call.args.size() >= 3 ? &a3 : nullptr,
                call.args.size() >= 4 ? &a4 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("ENCRYPT") || fname.equals("DECRYPT") || fname.equals("ENCRYPTBINARY") || fname.equals("DECRYPTBINARY")) {
            if (call.args.size() < 2 || call.args.size() > 6) throw webstrada::exception(fname + " requires 2 to 6 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3, a4, a5, a6;
            if (call.args.size() >= 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 5) a5 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 6) a6 = evaluateExpr(out, call.args[5], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("ENCRYPT")) {
                auto *ret = cfml::cf_encrypt(&a1, &a2,
                    call.args.size() >= 3 ? &a3 : nullptr,
                    call.args.size() >= 4 ? &a4 : nullptr,
                    call.args.size() >= 5 ? &a5 : nullptr,
                    call.args.size() >= 6 ? &a6 : nullptr);
                cf_register_temp(ret);
                return *ret;
            }
            if (fname.equals("DECRYPT")) {
                auto *ret = cfml::cf_decrypt(&a1, &a2,
                    call.args.size() >= 3 ? &a3 : nullptr,
                    call.args.size() >= 4 ? &a4 : nullptr,
                    call.args.size() >= 5 ? &a5 : nullptr,
                    call.args.size() >= 6 ? &a6 : nullptr);
                cf_register_temp(ret);
                return *ret;
            }
            if (fname.equals("ENCRYPTBINARY")) {
                auto *ret = cfml::cf_encryptbinary(&a1, &a2,
                    call.args.size() >= 3 ? &a3 : nullptr,
                    call.args.size() >= 4 ? &a4 : nullptr,
                    call.args.size() >= 5 ? &a5 : nullptr,
                    call.args.size() >= 6 ? &a6 : nullptr);
                cf_register_temp(ret);
                return *ret;
            }
            auto *ret = cfml::cf_decryptbinary(&a1, &a2,
                call.args.size() >= 3 ? &a3 : nullptr,
                call.args.size() >= 4 ? &a4 : nullptr,
                call.args.size() >= 5 ? &a5 : nullptr,
                call.args.size() >= 6 ? &a6 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("GENERATESECRETKEY")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("GenerateSecretKey requires 1 or 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() >= 2) a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_generatesecretkey(&a1, call.args.size() >= 2 ? &a2 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("GENERATE3DESKEY")) {
            if (call.args.size() != 1) throw webstrada::exception("Generate3DesKey requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_generate3deskey(&a1);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("GENERATEPBKDFKEY")) {
            if (call.args.size() != 5) throw webstrada::exception("GeneratePBKDFKey requires exactly 5 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a5 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_generatepbkdfkey(&a1, &a2, &a3, &a4, &a5);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("TOBASE64")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ToBase64 requires 1 or 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() >= 2) a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_tobase64(&a1, call.args.size() >= 2 ? &a2 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("TOBINARY")) {
            if (call.args.size() != 1) throw webstrada::exception("ToBinary requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_tobinary(&a1);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("BINARYENCODE")) {
            if (call.args.size() != 2) throw webstrada::exception("BinaryEncode requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_binaryencode(&a1, &a2);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("CHARSETDECODE")) {
            if (call.args.size() != 2) throw webstrada::exception("CharsetDecode requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_charsetdecode(&a1, &a2);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("CHARSETENCODE")) {
            if (call.args.size() != 2) throw webstrada::exception("CharsetEncode requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_charsetencode(&a1, &a2);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("URLDECODE") || fname.equals("URLENCODEDFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception(fname + " requires 1 or 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() >= 2) a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("URLDECODE")) {
                auto *ret = cfml::cf_urldecode(&a1, call.args.size() >= 2 ? &a2 : nullptr);
                cf_register_temp(ret);
                return *ret;
            }
            auto *ret = cfml::cf_urlencodedformat(&a1, call.args.size() >= 2 ? &a2 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("ENCODEFORURL")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("EncodeForURL requires 1 or 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() >= 2) a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_encodeforurl(&a1, call.args.size() >= 2 ? &a2 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("DECODEFROMURL")) {
            if (call.args.size() != 1) throw webstrada::exception("DecodeFromURL requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_decodefromurl(&a1);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("URLSESSIONFORMAT")) {
            if (call.args.size() != 1) throw webstrada::exception("URLSessionFormat requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_urlsessionformat(&a1);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("APPLICATIONSTOP") || fname.equals("GETAPPLICATIONMETADATA") ||
            fname.equals("SESSIONGETMETADATA") || fname.equals("SESSIONINVALIDATE") ||
            fname.equals("SESSIONROTATE")) {
            if (!call.args.empty()) throw webstrada::exception(fname + " requires 0 arguments");
            if (fname.equals("APPLICATIONSTOP")) return tempReturn(cfml::cf_applicationstop());
            if (fname.equals("GETAPPLICATIONMETADATA")) return tempReturn(cfml::cf_getapplicationmetadata());
            if (fname.equals("SESSIONGETMETADATA")) return tempReturn(cfml::cf_sessiongetmetadata());
            if (fname.equals("SESSIONINVALIDATE")) return tempReturn(cfml::cf_sessioninvalidate());
            return tempReturn(cfml::cf_sessionrotate());
        }

        if (fname.equals("TOSCRIPT")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("ToScript requires 2 to 4 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3, a4;
            if (call.args.size() >= 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_toscript(&a1, &a2, call.args.size() >= 3 ? &a3 : nullptr, call.args.size() >= 4 ? &a4 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("TOSTRING")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ToString requires 1 or 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2;
            if (call.args.size() >= 2) a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_tostring(&a1, call.args.size() >= 2 ? &a2 : nullptr);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("VAL")) {
            if (call.args.size() != 1) throw webstrada::exception("Val requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            auto *ret = cfml::cf_val(&a1);
            cf_register_temp(ret);
            return *ret;
        }

        if (fname.equals("DIRECTORYCOPY")) {
            if (call.args.size() != 2) throw webstrada::exception("DirectoryCopy requires exactly 2 arguments");
            cfvariant src = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dest = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_directorycopy(&src, &dest));
        }

        if (fname.equals("DIRECTORYRENAME")) {
            if (call.args.size() != 2) throw webstrada::exception("DirectoryRename requires exactly 2 arguments");
            cfvariant src = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dest = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_directoryrename(&src, &dest));
        }

        if (fname.equals("DIRECTORYLIST")) {
            if (call.args.size() < 1 || call.args.size() > 5) throw webstrada::exception("DirectoryList requires 1 to 5 arguments");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant recurse;
            if (call.args.size() >= 2) {
                recurse = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            cfvariant filter;
            if (call.args.size() >= 3) {
                filter = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            cfvariant sort;
            if (call.args.size() >= 4) {
                sort = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            }
            cfvariant type;
            if (call.args.size() >= 5) {
                type = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_directorylist(&path,
                call.args.size() >= 2 ? &recurse : nullptr,
                call.args.size() >= 3 ? &filter : nullptr,
                call.args.size() >= 4 ? &sort : nullptr,
                call.args.size() >= 5 ? &type : nullptr));
        }

        if (fname.equals("EXPANDPATH")) {
            if (call.args.size() != 1) throw webstrada::exception("ExpandPath requires exactly 1 argument");
            cfvariant path = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_expandpath(&path));
        }

        if (fname.equals("GETPROFILESECTIONS")) {
            if (call.args.size() != 1) throw webstrada::exception("GetProfileSections requires exactly 1 argument");
            cfvariant iniPath = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_getprofilesections(&iniPath));
        }

        if (fname.equals("GETPROFILESTRING")) {
            if (call.args.size() != 3) throw webstrada::exception("GetProfileString requires exactly 3 arguments");
            cfvariant iniPath = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant section = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant key = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_getprofilestring(&iniPath, &section, &key));
        }

        if (fname.equals("LISTLEN")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ListLen requires 1 or 2 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 2) {
                delim = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listlen(&list, call.args.size() == 2 ? &delim : nullptr));
        }

        if (fname.equals("LISTAPPEND")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListAppend requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listappend(&list, &val, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("LISTPREPEND")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListPrepend requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listprepend(&list, &val, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("LISTINSERTAT")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception("ListInsertAt requires 3 or 4 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant idx = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 4) {
                delim = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listinsertat(&list, &idx, &val, call.args.size() == 4 ? &delim : nullptr));
        }

        if (fname.equals("LISTDELETEAT")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListDeleteAt requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant idx = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listdeleteat(&list, &idx, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("LISTGETAT")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListGetAt requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant idx = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listgetat(&list, &idx, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("LISTSETAT")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception("ListSetAt requires 3 or 4 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant idx = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 4) {
                delim = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listsetat(&list, &idx, &val, call.args.size() == 4 ? &delim : nullptr));
        }

        if (fname.equals("LISTFIRST")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ListFirst requires 1 or 2 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 2) {
                delim = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listfirst(&list, call.args.size() == 2 ? &delim : nullptr));
        }

        if (fname.equals("LISTLAST")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ListLast requires 1 or 2 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 2) {
                delim = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listlast(&list, call.args.size() == 2 ? &delim : nullptr));
        }

        if (fname.equals("LISTREST")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ListRest requires 1 or 2 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 2) {
                delim = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listrest(&list, call.args.size() == 2 ? &delim : nullptr));
        }

        if (fname.equals("LISTFIND")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListFind requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listfind(&list, &val, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("LISTFINDNOCASE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListFindNoCase requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listfindnocase(&list, &val, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("LISTCONTAINS")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListContains requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listcontains(&list, &val, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("LISTCONTAINSNOCASE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListContainsNoCase requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listcontainsnocase(&list, &val, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("LISTCHANGEDELIMS")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("ListChangeDelims requires 2 or 3 arguments");
            cfvariant list = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant newDelim = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 3) {
                delim = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_listchangedelims(&list, &newDelim, call.args.size() == 3 ? &delim : nullptr));
        }

        if (fname.equals("DECIMALFORMAT")) {
            if (call.args.size() != 1) throw webstrada::exception("DecimalFormat requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_decimalformat(&arg));
        }

        if (fname.equals("DOLLARFORMAT")) {
            if (call.args.size() != 1) throw webstrada::exception("DollarFormat requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_dollarformat(&arg));
        }

        if (fname.equals("YESNOFORMAT")) {
            if (call.args.size() != 1) throw webstrada::exception("YesNoFormat requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_yesnoformat(&arg));
        }


        if (fname.equals("PARAGRAPHFORMAT")) {
            if (call.args.size() != 1) throw webstrada::exception("ParagraphFormat requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            string s = arg.toString();

            std::string stdStr;
            if (s.constData()) {
                stdStr = s.constData();
            }

            // Normalize CRLF and CR to LF
            size_t pos = 0;
            while ((pos = stdStr.find("\r\n", pos)) != std::string::npos) {
                stdStr.replace(pos, 2, "\n");
            }
            pos = 0;
            while ((pos = stdStr.find("\r", pos)) != std::string::npos) {
                stdStr.replace(pos, 1, "\n");
            }

            // Step 1: Replace double newlines \n\n with temporary placeholder _#P#_
            pos = 0;
            while ((pos = stdStr.find("\n\n", pos)) != std::string::npos) {
                stdStr.replace(pos, 2, "_#P#_");
                pos += 5;
            }

            // Step 2: Replace remaining single \n with a space " "
            pos = 0;
            while ((pos = stdStr.find("\n", pos)) != std::string::npos) {
                stdStr.replace(pos, 1, " ");
            }

            // Step 3: Replace placeholder _#P#_ with "  <P>\r\n"
            pos = 0;
            while ((pos = stdStr.find("_#P#_", pos)) != std::string::npos) {
                stdStr.replace(pos, 5, "  <P>\r\n");
                pos += 7;
            }

            // Step 4: Append " <P>"
            stdStr += " <P>";

            return cfvariant(string(stdStr.c_str()));
        }

        if (fname.equals("NOW")) {
            if (call.args.size() != 0) throw webstrada::exception("Now requires exactly 0 arguments");
            return tempReturn(cfml::cf_now());
        }

        if (fname.equals("CREATEDATETIME")) {
            if (call.args.size() != 6) throw webstrada::exception("CreateDateTime requires exactly 6 arguments");
            cfvariant yr = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant mon = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant day = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant hr = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            cfvariant min = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            cfvariant sec = evaluateExpr(out, call.args[5], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_createdatetime(&yr, &mon, &day, &hr, &min, &sec));
        }

        if (fname.equals("CREATEDATE")) {
            if (call.args.size() != 3) throw webstrada::exception("CreateDate requires exactly 3 arguments");
            cfvariant yr = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant mon = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant day = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_createdate(&yr, &mon, &day));
        }

        if (fname.equals("CREATETIME")) {
            if (call.args.size() != 3) throw webstrada::exception("CreateTime requires exactly 3 arguments");
            cfvariant hr = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant min = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant sec = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_createtime(&hr, &min, &sec));
        }

        if (fname.equals("CREATEODBCDATETIME")) {
            if (call.args.size() != 1) throw webstrada::exception("CreateODBCDateTime requires exactly 1 argument");
            cfvariant date = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_createodbcdatetime(&date));
        }


        if (fname.equals("DATEFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("DateFormat requires 1 or 2 arguments");
            cfvariant dateVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            double days = 0.0;
            if (dateVal.m_type == cfvariant::DateTime) {
                days = dateVal.m_double;
            } else if (dateVal.m_type == cfvariant::Number) {
                days = dateVal.m_int;
            } else if (dateVal.m_type == cfvariant::Long) {
                days = static_cast<double>(dateVal.m_long);
            } else if (dateVal.m_type == cfvariant::Float) {
                days = dateVal.m_double;
            } else {
                if (!parseDateTimeStr(dateVal.toString(), days)) {
                    throw webstrada::exception("DateFormat: Invalid date value");
                }
            }
            string maskStr = "";
            if (call.args.size() == 2) {
                maskStr = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables).toString();
            }
            return cfvariant(formatDateTime(days, maskStr, ModeDate));
        }

        if (fname.equals("TIMEFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("TimeFormat requires 1 or 2 arguments");
            cfvariant dateVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            double days = 0.0;
            if (dateVal.m_type == cfvariant::DateTime) {
                days = dateVal.m_double;
            } else if (dateVal.m_type == cfvariant::Number) {
                days = dateVal.m_int;
            } else if (dateVal.m_type == cfvariant::Long) {
                days = static_cast<double>(dateVal.m_long);
            } else if (dateVal.m_type == cfvariant::Float) {
                days = dateVal.m_double;
            } else {
                if (!parseDateTimeStr(dateVal.toString(), days)) {
                    throw webstrada::exception("TimeFormat: Invalid time value");
                }
            }
            string maskStr = "";
            if (call.args.size() == 2) {
                maskStr = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables).toString();
            }
            return cfvariant(formatDateTime(days, maskStr, ModeTime));
        }

        if (fname.equals("DATETIMEFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("DateTimeFormat requires 1 or 2 arguments");
            cfvariant dateVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            double days = 0.0;
            if (dateVal.m_type == cfvariant::DateTime) {
                days = dateVal.m_double;
            } else if (dateVal.m_type == cfvariant::Number) {
                days = dateVal.m_int;
            } else if (dateVal.m_type == cfvariant::Long) {
                days = static_cast<double>(dateVal.m_long);
            } else if (dateVal.m_type == cfvariant::Float) {
                days = dateVal.m_double;
            } else {
                if (!parseDateTimeStr(dateVal.toString(), days)) {
                    throw webstrada::exception("DateTimeFormat: Invalid date/time value");
                }
            }
            string maskStr = "";
            if (call.args.size() == 2) {
                maskStr = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables).toString();
            }
            return cfvariant(formatDateTime(days, maskStr, ModeDateTime));
        }

        if (fname.equals("ISDATE")) {
            if (call.args.size() != 1) throw webstrada::exception("IsDate requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isdate(&arg));
        }

        if (fname.equals("YEAR") || fname.equals("MONTH") || fname.equals("DAY") ||
            fname.equals("HOUR") || fname.equals("MINUTE") || fname.equals("SECOND")) {
            if (call.args.size() != 1) throw webstrada::exception("Datetime extractor requires exactly 1 argument");
            cfvariant dateVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("YEAR")) return tempReturn(cfml::cf_year(&dateVal));
            else if (fname.equals("MONTH")) return tempReturn(cfml::cf_month(&dateVal));
            else if (fname.equals("DAY")) return tempReturn(cfml::cf_day(&dateVal));
            else if (fname.equals("HOUR")) return tempReturn(cfml::cf_hour(&dateVal));
            else if (fname.equals("MINUTE")) return tempReturn(cfml::cf_minute(&dateVal));
            else return tempReturn(cfml::cf_second(&dateVal));
        }


        if (fname.equals("CREATETIMESPAN")) {
            if (call.args.size() != 4) throw webstrada::exception("CreateTimeSpan requires exactly 4 arguments");
            cfvariant d = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant h = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant m = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant s = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_createtimespan(&d, &h, &m, &s));
        }

        if (fname.equals("DATEADD")) {
            if (call.args.size() != 3) throw webstrada::exception("DateAdd requires exactly 3 arguments");
            cfvariant dp = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant num = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dt = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_dateadd(&dp, &num, &dt));
        }

        if (fname.equals("DATECOMPARE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("DateCompare requires 2 or 3 arguments");
            cfvariant dt1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dt2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dp = (call.args.size() == 3) ? evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_datecompare(&dt1, &dt2, &dp));
        }

        if (fname.equals("DATECONVERT")) {
            if (call.args.size() != 2) throw webstrada::exception("DateConvert requires exactly 2 arguments");
            cfvariant typ = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dt = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_dateconvert(&typ, &dt));
        }

        if (fname.equals("DATEDIFF")) {
            if (call.args.size() != 3) throw webstrada::exception("DateDiff requires exactly 3 arguments");
            cfvariant dp = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dt1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dt2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_datediff(&dp, &dt1, &dt2));
        }

        if (fname.equals("DATEPART")) {
            if (call.args.size() != 2) throw webstrada::exception("DatePart requires exactly 2 arguments");
            cfvariant dp = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant dt = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_datepart(&dp, &dt));
        }

        if (fname.equals("DAYOFWEEK")) {
            if (call.args.size() != 1) throw webstrada::exception("DayOfWeek requires exactly 1 argument");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_dayofweek(&dt));
        }

        if (fname.equals("DAYOFWEEKASSTRING")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("DayOfWeekAsString requires 1 or 2 arguments");
            cfvariant day = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant loc = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_dayofweekasstring(&day, &loc));
        }

        if (fname.equals("DAYOFYEAR")) {
            if (call.args.size() != 1) throw webstrada::exception("DayOfYear requires exactly 1 argument");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_dayofyear(&dt));
        }

        if (fname.equals("DAYSINMONTH")) {
            if (call.args.size() != 1) throw webstrada::exception("DaysInMonth requires exactly 1 argument");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_daysinmonth(&dt));
        }

        if (fname.equals("DAYSINYEAR")) {
            if (call.args.size() != 1) throw webstrada::exception("DaysInYear requires exactly 1 argument");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_daysinyear(&dt));
        }

        if (fname.equals("FIRSTDAYOFMONTH")) {
            if (call.args.size() != 1) throw webstrada::exception("FirstDayOfMonth requires exactly 1 argument");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_firstdayofmonth(&dt));
        }

        if (fname.equals("GETHTTPTIMESTRING")) {
            if (call.args.size() > 1) throw webstrada::exception("GetHttpTimeString requires 0 or 1 arguments");
            cfvariant dt = (call.args.size() == 1) ? evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_gethttptimestring(&dt));
        }

        if (fname.equals("GETHTTPREQUESTDATA")) {
            if (call.args.size() > 1) throw webstrada::exception("GetHttpRequestData requires 0 or 1 arguments");
            cfvariant includeBody(cfvariant::Null);
            if (call.args.size() == 1)
                includeBody = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_gethttprequestdata(cgi, &includeBody));
        }

        if (fname.equals("GETTIMEZONEINFO")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("GetTimeZoneInfo requires 0 arguments");
            }
            return tempReturn(cfml::cf_gettimezoneinfo());
        }

        if (fname.equals("ISBINARY")) {
            if (call.args.size() != 1) throw webstrada::exception("IsBinary requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isbinary(&val));
        }

        if (fname.equals("ISBOOLEAN")) {
            if (call.args.size() != 1) throw webstrada::exception("IsBoolean requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isboolean(&val));
        }

        if (fname.equals("ISCLOSURE")) {
            if (call.args.size() != 1) throw webstrada::exception("IsClosure requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isclosure(&val));
        }

        if (fname.equals("ISCUSTOMFUNCTION")) {
            if (call.args.size() != 1) throw webstrada::exception("IsCustomFunction requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_iscustomfunction(&val));
        }

        if (fname.equals("ISDEFINED")) {
            if (call.args.size() != 1) throw webstrada::exception("IsDefined requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isdefined(&val, cgi, server, cookie, application, session, url, form, variables));
        }

        if (fname.equals("ISFILEOBJECT")) {
            if (call.args.size() != 1) throw webstrada::exception("IsFileObject requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isfileobject(&val));
        }

        if (fname.equals("ISIMAGE")) {
            if (call.args.size() != 1) throw webstrada::exception("IsImage requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isimage(&val));
        }

        if (fname.equals("ISNULL")) {
            if (call.args.size() != 1) throw webstrada::exception("IsNull requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isnull(&val));
        }

        if (fname.equals("ISNUMERIC")) {
            if (call.args.size() != 1) throw webstrada::exception("IsNumeric requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isnumeric(&val));
        }

        if (fname.equals("LSISNUMERIC")) {
            if (call.args.size() != 1) throw webstrada::exception("LSIsNumeric requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_lsisnumeric(&val));
        }

        if (fname.equals("ISOBJECT")) {
            if (call.args.size() != 1) throw webstrada::exception("IsObject requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isobject(&val));
        }

        if (fname.equals("ISSIMPLEVALUE")) {
            if (call.args.size() != 1) throw webstrada::exception("IsSimpleValue requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_issimplevalue(&val));
        }

        if (fname.equals("ISSTRUCT")) {
            if (call.args.size() != 1) throw webstrada::exception("IsStruct requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isstruct(&val));
        }

        if (fname.equals("FILEISEOF")) {
            if (call.args.size() != 1) throw webstrada::exception("FileIsEOF requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_fileiseof(&val));
        }

        if (fname.equals("ISDATEOBJECT")) {
            if (call.args.size() != 1) throw webstrada::exception("IsDateObject requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isdateobject(&val));
        }

        if (fname.equals("ISLEAPYEAR")) {
            if (call.args.size() != 1) throw webstrada::exception("IsLeapYear requires exactly 1 argument");
            cfvariant yr = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isleapyear(&yr));
        }

        if (fname.equals("ISNUMERICDATE")) {
            if (call.args.size() != 1) throw webstrada::exception("IsNumericDate requires exactly 1 argument");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isnumericdate(&val));
        }

        if (fname.equals("LSDATEFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("LSDateFormat requires between 1 and 3 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant msk = (call.args.size() >= 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            cfvariant loc = (call.args.size() == 3) ? evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsdateformat(&dt, &msk, &loc));
        }

        if (fname.equals("LSDATETIMEFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("LSDateTimeFormat requires between 1 and 3 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant msk = (call.args.size() >= 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            cfvariant loc = (call.args.size() == 3) ? evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsdatetimeformat(&dt, &msk, &loc));
        }

        if (fname.equals("LSISDATE")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("LSIsDate requires 1 or 2 arguments");
            cfvariant val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant loc = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsisdate(&val, &loc));
        }

        if (fname.equals("LSPARSEDATETIME")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("LSParseDateTime requires 1 or 2 arguments");
            cfvariant str = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant loc = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsparsedatetime(&str, &loc));
        }

        if (fname.equals("LSTIMEFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("LSTimeFormat requires between 1 and 3 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant msk = (call.args.size() >= 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            cfvariant loc = (call.args.size() == 3) ? evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lstimeformat(&dt, &msk, &loc));
        }

        if (fname.equals("MONTHASSTRING")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("MonthAsString requires 1 or 2 arguments");
            cfvariant num = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant loc = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_monthasstring(&num, &loc));
        }

        if (fname.equals("PARSEDATETIME")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ParseDateTime requires 1 or 2 arguments");
            cfvariant str = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant pop = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_parsedatetime(&str, &pop));
        }

        if (fname.equals("QUARTER")) {
            if (call.args.size() != 1) throw webstrada::exception("Quarter requires exactly 1 argument");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_quarter(&dt));
        }

        if (fname.equals("SETDAY")) {
            if (call.args.size() != 2) throw webstrada::exception("SetDay requires exactly 2 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_setday(&dt, &val));
        }

        if (fname.equals("SETHOUR")) {
            if (call.args.size() != 2) throw webstrada::exception("SetHour requires exactly 2 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_sethour(&dt, &val));
        }

        if (fname.equals("SETMINUTE")) {
            if (call.args.size() != 2) throw webstrada::exception("SetMinute requires exactly 2 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_setminute(&dt, &val));
        }

        if (fname.equals("SETMONTH")) {
            if (call.args.size() != 2) throw webstrada::exception("SetMonth requires exactly 2 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_setmonth(&dt, &val));
        }

        if (fname.equals("SETSECOND")) {
            if (call.args.size() != 2) throw webstrada::exception("SetSecond requires exactly 2 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_setsecond(&dt, &val));
        }

        if (fname.equals("SETYEAR")) {
            if (call.args.size() != 2) throw webstrada::exception("SetYear requires exactly 2 arguments");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_setyear(&dt, &val));
        }

        if (fname.equals("WEEK")) {
            if (call.args.size() != 1) throw webstrada::exception("Week requires exactly 1 argument");
            cfvariant dt = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_week(&dt));
        }


        if (fname.equals("ABS")) {
            if (call.args.size() != 1) throw webstrada::exception("Abs requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_abs(&arg));
        }

        if (fname.equals("GETREADABLEIMAGEFORMATS")) {
            if (call.args.size() != 0) throw webstrada::exception("GetReadableImageFormats requires exactly 0 arguments");
            return tempReturn(cfml::cf_getreadableimageformats());
        }

        if (fname.equals("GETWRITEABLEIMAGEFORMATS")) {
            if (call.args.size() != 0) throw webstrada::exception("GetWriteableImageFormats requires exactly 0 arguments");
            return tempReturn(cfml::cf_getwriteableimageformats());
        }

        if (fname.equals("IMAGEREAD") || fname.equals("IMAGEREADBASE64") ||
            fname.equals("IMAGEGETBLOB") || fname.equals("IMAGEGETWIDTH") ||
            fname.equals("IMAGEGETHEIGHT") || fname.equals("IMAGEINFO") ||
            fname.equals("IMAGEGETMETADATA")) {
            if (call.args.size() != 1) throw webstrada::exception(call.name + " requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("IMAGEREAD")) return tempReturn(cfml::cf_imageread(&arg));
            if (fname.equals("IMAGEREADBASE64")) return tempReturn(cfml::cf_imagereadbase64(&arg));
            if (fname.equals("IMAGEGETBLOB")) return tempReturn(cfml::cf_imagegetblob(&arg));
            if (fname.equals("IMAGEGETWIDTH")) return tempReturn(cfml::cf_imagegetwidth(&arg));
            if (fname.equals("IMAGEGETHEIGHT")) return tempReturn(cfml::cf_imagegetheight(&arg));
            if (fname.equals("IMAGEINFO")) return tempReturn(cfml::cf_imageinfo(&arg));
            return tempReturn(cfml::cf_imagegetmetadata(&arg));
        }

        if (fname.equals("IMAGEGETBUFFEREDIMAGE") || fname.equals("IMAGEGETEXIFMETADATA") ||
            fname.equals("IMAGEGETIPTCMETADATA")) {
            if (call.args.size() != 1) throw webstrada::exception(call.name + " requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("IMAGEGETBUFFEREDIMAGE")) return tempReturn(cfml::cf_imagegetbufferedimage(&arg));
            if (fname.equals("IMAGEGETEXIFMETADATA")) return tempReturn(cfml::cf_imagegetexifmetadata(&arg));
            return tempReturn(cfml::cf_imagegetiptcmetadata(&arg));
        }

        if (fname.equals("IMAGEGETEXIFTAG") || fname.equals("IMAGEGETIPTCTAG")) {
            if (call.args.size() != 2) throw webstrada::exception(call.name + " requires exactly 2 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("IMAGEGETEXIFTAG")) return tempReturn(cfml::cf_imagegetexiftag(&arg0, &arg1));
            return tempReturn(cfml::cf_imagegetiptctag(&arg0, &arg1));
        }

        if (fname.equals("IMAGECREATECAPTCHA")) {
            if (call.args.size() < 3 || call.args.size() > 6)
                throw webstrada::exception("ImageCreateCaptcha requires 3 to 6 arguments");
            cfvariant args[6];
            for (size_t i = 0; i < 6; i++) {
                if (i < call.args.size())
                    args[i] = evaluateExpr(out, call.args[i], cgi, server, cookie, application, session, url, form, variables);
                else
                    args[i] = cfvariant(cfvariant::Null);
            }
            return tempReturn(cfml::cf_imagecreatecaptcha(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5]));
        }

        if (fname.equals("IMAGEWRITE") || fname.equals("IMAGEWRITEBASE64") || fname.equals("IMAGENEW") || fname.equals("ISIMAGEFILE")) {
            size_t maxArgs = (fname.equals("IMAGEWRITE")) ? 4 : ((fname.equals("IMAGEWRITEBASE64")) ? 5 : ((fname.equals("ISIMAGEFILE")) ? 2 : 5));
            size_t minArgs = (fname.equals("IMAGENEW")) ? 0 : ((fname.equals("ISIMAGEFILE")) ? 1 : 2);
            if (call.args.size() < minArgs || call.args.size() > maxArgs)
                throw webstrada::exception(call.name + " requires " + webstrada::string(std::to_string(minArgs).c_str()) + " to " + webstrada::string(std::to_string(maxArgs).c_str()) + " arguments");
            cfvariant args[5];
            for (size_t i = 0; i < maxArgs; i++) {
                if (i < call.args.size())
                    args[i] = evaluateExpr(out, call.args[i], cgi, server, cookie, application, session, url, form, variables);
                else
                    args[i] = cfvariant(cfvariant::Null);
            }
            if (fname.equals("IMAGEWRITE"))
                return tempReturn(cfml::cf_imagewrite(&args[0], &args[1], &args[2], &args[3]));
            if (fname.equals("IMAGEWRITEBASE64"))
                return tempReturn(cfml::cf_imagewritebase64(&args[0], &args[1], &args[2], &args[3], &args[4]));
            if (fname.equals("ISIMAGEFILE"))
                return tempReturn(cfml::cf_isimagefile(&args[0], &args[1]));
            return tempReturn(cfml::cf_imagenew(&args[0], &args[1], &args[2], &args[3], &args[4]));
        }

        if (fname.equals("IMAGECLEARRECT") || fname.equals("IMAGEDRAWLINE") || fname.equals("IMAGEDRAWLINES") ||
            fname.equals("IMAGEDRAWRECT") || fname.equals("IMAGEDRAWROUNDRECT") || fname.equals("IMAGEDRAWBEVELEDRECT") ||
            fname.equals("IMAGEDRAWOVAL") || fname.equals("IMAGEDRAWARC") || fname.equals("IMAGEDRAWCUBICCURVE") ||
            fname.equals("IMAGEDRAWQUADRATICCURVE") || fname.equals("IMAGEDRAWPOINT") || fname.equals("IMAGEDRAWTEXT") ||
            fname.equals("IMAGESETANTIALIASING") || fname.equals("IMAGESETBACKGROUNDCOLOR") ||
            fname.equals("IMAGESETDRAWINGCOLOR") || fname.equals("IMAGESETDRAWINGSTROKE") ||
            fname.equals("IMAGESETDRAWINGTRANSPARENCY") || fname.equals("IMAGEXORDRAWINGMODE") ||
            fname.equals("IMAGEROTATEDRAWINGAXIS") || fname.equals("IMAGESHEARDRAWINGAXIS") ||
            fname.equals("IMAGETRANSLATEDRAWINGAXIS")) {
            size_t maxArgs = (fname.equals("IMAGEDRAWCUBICCURVE")) ? 9 :
                             (fname.equals("IMAGEDRAWARC") || fname.equals("IMAGEDRAWROUNDRECT")) ? 8 :
                             (fname.equals("IMAGEDRAWBEVELEDRECT") || fname.equals("IMAGEDRAWQUADRATICCURVE")) ? 7 :
                             (fname.equals("IMAGEDRAWOVAL") || fname.equals("IMAGEDRAWRECT")) ? 6 :
                             (fname.equals("IMAGECLEARRECT") || fname.equals("IMAGEDRAWLINE") || fname.equals("IMAGEDRAWLINES")) ? 5 :
                             (fname.equals("IMAGEDRAWPOINT")) ? 3 :
                             (fname.equals("IMAGEDRAWTEXT")) ? 5 :
                             (fname.equals("IMAGEROTATEDRAWINGAXIS")) ? 4 :
                             (fname.equals("IMAGESHEARDRAWINGAXIS") || fname.equals("IMAGETRANSLATEDRAWINGAXIS")) ? 3 : 2;
            size_t minArgs = (fname.equals("IMAGESETANTIALIASING") || fname.equals("IMAGESETDRAWINGSTROKE")) ? 1 :
                             (fname.equals("IMAGEDRAWARC") || fname.equals("IMAGEDRAWROUNDRECT")) ? 7 :
                             (fname.equals("IMAGEDRAWBEVELEDRECT")) ? 6 :
                             (fname.equals("IMAGEDRAWOVAL") || fname.equals("IMAGEDRAWRECT")) ? 5 :
                             (fname.equals("IMAGEDRAWLINES")) ? 3 :
                             (fname.equals("IMAGESHEARDRAWINGAXIS") || fname.equals("IMAGETRANSLATEDRAWINGAXIS")) ? 3 :
                             (fname.equals("IMAGEDRAWTEXT")) ? 4 :
                             (fname.equals("IMAGEROTATEDRAWINGAXIS")) ? 2 : maxArgs;
            if (call.args.size() < minArgs || call.args.size() > maxArgs)
                throw webstrada::exception(call.name + " requires " + webstrada::string(std::to_string(minArgs).c_str()) + " to " + webstrada::string(std::to_string(maxArgs).c_str()) + " arguments");
            cfvariant args[9];
            for (size_t i = 0; i < maxArgs; i++) {
                if (i < call.args.size())
                    args[i] = evaluateExpr(out, call.args[i], cgi, server, cookie, application, session, url, form, variables);
                else
                    args[i] = cfvariant(cfvariant::Null);
            }
            if (fname.equals("IMAGECLEARRECT"))
                return tempReturn(cfml::cf_imageclearrect(&args[0], &args[1], &args[2], &args[3], &args[4]));
            if (fname.equals("IMAGEDRAWARC"))
                return tempReturn(cfml::cf_imagedrawarc(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5], &args[6], &args[7]));
            if (fname.equals("IMAGEDRAWBEVELEDRECT"))
                return tempReturn(cfml::cf_imagedrawbeveledrect(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5], &args[6]));
            if (fname.equals("IMAGEDRAWCUBICCURVE"))
                return tempReturn(cfml::cf_imagedrawcubiccurve(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5], &args[6], &args[7], &args[8]));
            if (fname.equals("IMAGEDRAWLINE"))
                return tempReturn(cfml::cf_imagedrawline(&args[0], &args[1], &args[2], &args[3], &args[4]));
            if (fname.equals("IMAGEDRAWLINES"))
                return tempReturn(cfml::cf_imagedrawlines(&args[0], &args[1], &args[2], &args[3], &args[4]));
            if (fname.equals("IMAGEDRAWOVAL"))
                return tempReturn(cfml::cf_imagedrawoval(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5]));
            if (fname.equals("IMAGEDRAWPOINT"))
                return tempReturn(cfml::cf_imagedrawpoint(&args[0], &args[1], &args[2]));
            if (fname.equals("IMAGEDRAWQUADRATICCURVE"))
                return tempReturn(cfml::cf_imagedrawquadraticcurve(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5], &args[6]));
            if (fname.equals("IMAGEDRAWRECT"))
                return tempReturn(cfml::cf_imagedrawrect(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5]));
            if (fname.equals("IMAGEDRAWROUNDRECT"))
                return tempReturn(cfml::cf_imagedrawroundrect(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5], &args[6], &args[7]));
            if (fname.equals("IMAGEDRAWTEXT"))
                return tempReturn(cfml::cf_imagedrawtext(&args[0], &args[1], &args[2], &args[3], &args[4]));
            if (fname.equals("IMAGEROTATEDRAWINGAXIS"))
                return tempReturn(cfml::cf_imagerotatedrawingaxis(&args[0], &args[1], &args[2], &args[3]));
            if (fname.equals("IMAGESHEARDRAWINGAXIS"))
                return tempReturn(cfml::cf_imagesheardrawingaxis(&args[0], &args[1], &args[2]));
            if (fname.equals("IMAGETRANSLATEDRAWINGAXIS"))
                return tempReturn(cfml::cf_imagetranslatedrawingaxis(&args[0], &args[1], &args[2]));
            if (fname.equals("IMAGESETANTIALIASING"))
                return tempReturn(cfml::cf_imagesetantialiasing(&args[0], &args[1]));
            if (fname.equals("IMAGESETBACKGROUNDCOLOR"))
                return tempReturn(cfml::cf_imagesetbackgroundcolor(&args[0], &args[1]));
            if (fname.equals("IMAGESETDRAWINGCOLOR"))
                return tempReturn(cfml::cf_imagesetdrawingcolor(&args[0], &args[1]));
            if (fname.equals("IMAGESETDRAWINGSTROKE"))
                return tempReturn(cfml::cf_imagesetdrawingstroke(&args[0], &args[1]));
            if (fname.equals("IMAGESETDRAWINGTRANSPARENCY"))
                return tempReturn(cfml::cf_imagesetdrawingtransparency(&args[0], &args[1]));
            return tempReturn(cfml::cf_imagexordrawingmode(&args[0], &args[1]));
        }

        if (fname.equals("IMAGEADDBORDER") || fname.equals("IMAGEBLUR") || fname.equals("IMAGECOPY") ||
            fname.equals("IMAGECROP") || fname.equals("IMAGEFLIP") || fname.equals("IMAGEGRAYSCALE") ||
            fname.equals("IMAGEMAKECOLORTRANSPARENT") || fname.equals("IMAGEMAKETRANSLUCENT") ||
            fname.equals("IMAGENEGATIVE") || fname.equals("IMAGEOVERLAY") || fname.equals("IMAGEPASTE") ||
            fname.equals("IMAGERESIZE") || fname.equals("IMAGEROTATE") || fname.equals("IMAGESCALETOFIT") ||
            fname.equals("IMAGESHARPEN") || fname.equals("IMAGESHEAR") || fname.equals("IMAGETRANSLATE")) {
            size_t maxArgs = (fname.equals("IMAGERESIZE") || fname.equals("IMAGESCALETOFIT")) ? 5 :
                             (fname.equals("IMAGECOPY")) ? 7 :
                             (fname.equals("IMAGEROTATE") || fname.equals("IMAGESHEAR")) ? 5 :
                             (fname.equals("IMAGEADDBORDER")) ? 4 :
                             (fname.equals("IMAGEOVERLAY")) ? 4 :
                             (fname.equals("IMAGETRANSLATE")) ? 4 :
                             (fname.equals("IMAGEPASTE") || fname.equals("IMAGECROP")) ? 5 :
                             (fname.equals("IMAGEFLIP") || fname.equals("IMAGEBLUR") ||
                              fname.equals("IMAGEMAKETRANSLUCENT") || fname.equals("IMAGESHARPEN")) ? 2 :
                             (fname.equals("IMAGEGRAYSCALE") || fname.equals("IMAGENEGATIVE")) ? 1 : 2;
            size_t minArgs = (fname.equals("IMAGEADDBORDER")) ? 2 :
                             (fname.equals("IMAGECOPY") || fname.equals("IMAGECROP")) ? 5 :
                             (fname.equals("IMAGEROTATE")) ? 2 :
                             (fname.equals("IMAGESHEAR")) ? 2 :
                             (fname.equals("IMAGEOVERLAY") || fname.equals("IMAGEPASTE")) ? 4 :
                             (fname.equals("IMAGERESIZE")) ? 3 :
                             (fname.equals("IMAGESCALETOFIT")) ? 3 :
                             (fname.equals("IMAGETRANSLATE")) ? 3 :
                             (fname.equals("IMAGEBLUR") || fname.equals("IMAGEFLIP") ||
                              fname.equals("IMAGEMAKETRANSLUCENT")) ? 1 :
                             (fname.equals("IMAGEGRAYSCALE") || fname.equals("IMAGENEGATIVE")) ? 1 : maxArgs;
            if (call.args.size() < minArgs || call.args.size() > maxArgs)
                throw webstrada::exception(call.name + " requires " + webstrada::string(std::to_string(minArgs).c_str()) + " to " + webstrada::string(std::to_string(maxArgs).c_str()) + " arguments");
            cfvariant args[7];
            for (size_t i = 0; i < maxArgs; i++) {
                if (i < call.args.size())
                    args[i] = evaluateExpr(out, call.args[i], cgi, server, cookie, application, session, url, form, variables);
                else
                    args[i] = cfvariant(cfvariant::Null);
            }
            if (fname.equals("IMAGEADDBORDER"))
                return tempReturn(cfml::cf_imageaddborder(&args[0], &args[1], &args[2], &args[3]));
            if (fname.equals("IMAGEBLUR"))
                return tempReturn(cfml::cf_imageblur(&args[0], &args[1]));
            if (fname.equals("IMAGECOPY"))
                return tempReturn(cfml::cf_imagecopy(&args[0], &args[1], &args[2], &args[3], &args[4], &args[5], &args[6]));
            if (fname.equals("IMAGECROP"))
                return tempReturn(cfml::cf_imagecrop(&args[0], &args[1], &args[2], &args[3], &args[4]));
            if (fname.equals("IMAGEFLIP"))
                return tempReturn(cfml::cf_imageflip(&args[0], &args[1]));
            if (fname.equals("IMAGEGRAYSCALE"))
                return tempReturn(cfml::cf_imagegrayscale(&args[0]));
            if (fname.equals("IMAGEMAKECOLORTRANSPARENT"))
                return tempReturn(cfml::cf_imagemakecolortransparent(&args[0], &args[1]));
            if (fname.equals("IMAGEMAKETRANSLUCENT"))
                return tempReturn(cfml::cf_imagemaketranslucent(&args[0], &args[1]));
            if (fname.equals("IMAGENEGATIVE"))
                return tempReturn(cfml::cf_imagenegative(&args[0]));
            if (fname.equals("IMAGEOVERLAY"))
                return tempReturn(cfml::cf_imageoverlay(&args[0], &args[1], &args[2], &args[3]));
            if (fname.equals("IMAGEPASTE"))
                return tempReturn(cfml::cf_imagepaste(&args[0], &args[1], &args[2], &args[3]));
            if (fname.equals("IMAGERESIZE"))
                return tempReturn(cfml::cf_imageresize(&args[0], &args[1], &args[2], &args[3], &args[4]));
            if (fname.equals("IMAGEROTATE")) {
                // CF signature: imageRotate(name [, x] [, y], angle [, interpolation]).
                // CFPage quirk (verified in decompiled source): the 4-arg form
                // (name, x, y, angle) IGNORES x/y and rotates about the center;
                // only the 5-arg form uses them. 2-arg = (name, angle); 3-arg =
                // (name, angle, interpolation).
                cfvariant nullV(cfvariant::Null);
                const cfvariant *x = &nullV, *y = &nullV, *ang, *interp = &nullV;
                if (call.args.size() == 2)      { ang = &args[1]; }
                else if (call.args.size() == 3) { ang = &args[1]; interp = &args[2]; }
                else if (call.args.size() == 4) { ang = &args[3]; } // x,y ignored
                else                            { x = &args[1]; y = &args[2]; ang = &args[3]; interp = &args[4]; }
                return tempReturn(cfml::cf_imagerotate(&args[0], x, y, ang, interp));
            }
            if (fname.equals("IMAGESCALETOFIT"))
                return tempReturn(cfml::cf_imagescaletofit(&args[0], &args[1], &args[2], &args[3], &args[4]));
            if (fname.equals("IMAGESHARPEN"))
                return tempReturn(cfml::cf_imagesharpen(&args[0], &args[1]));
            if (fname.equals("IMAGESHEAR"))
                return tempReturn(cfml::cf_imageshear(&args[0], &args[1], &args[2], &args[3]));
            return tempReturn(cfml::cf_imagetranslate(&args[0], &args[1], &args[2], &args[3]));
        }

        if (fname.equals("ASC")) {
            if (call.args.size() != 1) throw webstrada::exception("Asc requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_asc(&arg));
        }

        if (fname.equals("CHR")) {
            if (call.args.size() != 1) throw webstrada::exception("Chr requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_chr(&arg));
        }

        if (fname.equals("ACOS")) {
            if (call.args.size() != 1) throw webstrada::exception("Acos requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_acos(&arg));
        }

        if (fname.equals("ASIN")) {
            if (call.args.size() != 1) throw webstrada::exception("Asin requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_asin(&arg));
        }

        if (fname.equals("ATAN") || fname.equals("ATN")) {
            if (call.args.size() != 1) throw webstrada::exception("Atan requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_atan(&arg));
        }

        if (fname.equals("ATAN2")) {
            if (call.args.size() != 2) throw webstrada::exception("Atan2 requires exactly 2 arguments");
            cfvariant yVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant xVal = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_atan2(&yVal, &xVal));
        }

        if (fname.equals("CEILING")) {
            if (call.args.size() != 1) throw webstrada::exception("Ceiling requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_ceiling(&arg));
        }

        if (fname.equals("COS")) {
            if (call.args.size() != 1) throw webstrada::exception("Cos requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_cos(&arg));
        }

        if (fname.equals("EXP")) {
            if (call.args.size() != 1) throw webstrada::exception("Exp requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_exp(&arg));
        }

        if (fname.equals("BOOLEANFORMAT")) {
            if (call.args.size() != 1) throw webstrada::exception("BooleanFormat requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_booleanformat(&a1));
        }
        if (fname.equals("CJUSTIFY")) {
            if (call.args.size() != 2) throw webstrada::exception("CJustify requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_cjustify(&a1, &a2));
        }
        if (fname.equals("LJUSTIFY")) {
            if (call.args.size() != 2) throw webstrada::exception("LJustify requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_ljustify(&a1, &a2));
        }
        if (fname.equals("RJUSTIFY")) {
            if (call.args.size() != 2) throw webstrada::exception("RJustify requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_rjustify(&a1, &a2));
        }
        if (fname.equals("DE")) {
            if (call.args.size() != 1) throw webstrada::exception("DE requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_de(&a1));
        }
        if (fname.equals("REESCAPE")) {
            if (call.args.size() != 1) throw webstrada::exception("ReEscape requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_reescape(&a1));
        }
        if (fname.equals("REFIND") || fname.equals("REFINDNOCASE")) {
            if (call.args.size() < 2 || call.args.size() > 5) throw webstrada::exception(fname + " requires 2 to 5 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3, a4, a5;
            if (call.args.size() >= 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 5) a5 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("REFIND")) {
                return tempReturn(cfml::cf_refind(&a1, &a2, call.args.size() >= 3 ? &a3 : nullptr,
                                        call.args.size() >= 4 ? &a4 : nullptr,
                                        call.args.size() == 5 ? &a5 : nullptr));
            }
            return tempReturn(cfml::cf_refindnocase(&a1, &a2, call.args.size() >= 3 ? &a3 : nullptr,
                                          call.args.size() >= 4 ? &a4 : nullptr,
                                          call.args.size() == 5 ? &a5 : nullptr));
        }
        if (fname.equals("REMATCH") || fname.equals("REMATCHNOCASE")) {
            if (call.args.size() != 2) throw webstrada::exception(fname + " requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("REMATCH")) return tempReturn(cfml::cf_rematch(&a1, &a2));
            return tempReturn(cfml::cf_rematchnocase(&a1, &a2));
        }
        if (fname.equals("REREPLACE") || fname.equals("REREPLACENOCASE")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception(fname + " requires 3 or 4 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a4;
            if (call.args.size() == 4) a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("REREPLACE")) {
                return tempReturn(cfml::cf_rereplace(&a1, &a2, &a3, call.args.size() == 4 ? &a4 : nullptr));
            }
            return tempReturn(cfml::cf_rereplacenocase(&a1, &a2, &a3, call.args.size() == 4 ? &a4 : nullptr));
        }
        if (fname.equals("WRAP")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("Wrap requires 2 or 3 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3;
            if (call.args.size() == 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_wrap(&a1, &a2, call.args.size() == 3 ? &a3 : nullptr));
        }
        if (fname.equals("FORMATBASEN")) {
            if (call.args.size() != 2) throw webstrada::exception("FormatBaseN requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_formatbasen(&a1, &a2));
        }
        if (fname.equals("INPUTBASEN")) {
            if (call.args.size() != 2) throw webstrada::exception("InputBaseN requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_inputbasen(&a1, &a2));
        }
        if (fname.equals("JSSTRINGFORMAT")) {
            if (call.args.size() != 1) throw webstrada::exception("JSStringFormat requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_jsstringformat(&a1));
        }
        if (fname.equals("REMOVECHARS")) {
            if (call.args.size() != 3) throw webstrada::exception("RemoveChars requires exactly 3 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_removechars(&a1, &a2, &a3));
        }
        if (fname.equals("SPANEXCLUDING")) {
            if (call.args.size() != 2) throw webstrada::exception("SpanExcluding requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_spanexcluding(&a1, &a2));
        }
        if (fname.equals("SPANINCLUDING")) {
            if (call.args.size() != 2) throw webstrada::exception("SpanIncluding requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_spanincluding(&a1, &a2));
        }
        if (fname.equals("STRIPCR")) {
            if (call.args.size() != 1) throw webstrada::exception("StripCR requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_stripcr(&a1));
        }
        if (fname.equals("HTMLEDITFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("HTMLEditFormat requires 1 or 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *a2 = nullptr;
            cfvariant tmp2;
            if (call.args.size() == 2) {
                tmp2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
                a2 = &tmp2;
            }
            return tempReturn(cfml::cf_htmleditformat(&a1, a2));
        }
        if (fname.equals("HTMLCODEFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("HTMLCodeFormat requires 1 or 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *a2 = nullptr;
            cfvariant tmp2;
            if (call.args.size() == 2) {
                tmp2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
                a2 = &tmp2;
            }
            return tempReturn(cfml::cf_htmlcodeformat(&a1, a2));
        }
        if (fname.equals("INVOKE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("Invoke requires 2 or 3 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *a3 = nullptr;
            cfvariant tmp3;
            if (call.args.size() == 3) {
                tmp3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
                a3 = &tmp3;
            }
            return tempReturn(cfml::cf_invoke(&a1, &a2, a3, out, cgi, server, cookie, application, session, url, form, variables));
        }
        if (fname.equals("AJAXLINK")) {
            if (call.args.size() != 1) throw webstrada::exception("AjaxLink requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_ajaxlink(&a1));
        }
        if (fname.equals("AJAXONLOAD")) {
            if (call.args.size() != 1) throw webstrada::exception("AjaxOnLoad requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_ajaxonload(&a1, out));
        }
        if (fname.equals("INVOKECFCLIENTFUNCTION")) {
            throw webstrada::exception("Variable INVOKECFCLIENTFUNCTION is undefined.");
        }
        if (fname.equals("REPLACELIST")) {
            if (call.args.size() < 3 || call.args.size() > 6) throw webstrada::exception("ReplaceList requires 3 to 6 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *a4 = nullptr, *a5 = nullptr, *a6 = nullptr;
            cfvariant tmp4, tmp5, tmp6;
            if (call.args.size() >= 4) {
                tmp4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
                a4 = &tmp4;
            }
            if (call.args.size() >= 5) {
                tmp5 = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
                a5 = &tmp5;
            }
            if (call.args.size() == 6) {
                tmp6 = evaluateExpr(out, call.args[5], cgi, server, cookie, application, session, url, form, variables);
                a6 = &tmp6;
            }
            return tempReturn(cfml::cf_replacelist(&a1, &a2, &a3, a4, a5, a6));
        }
        if (fname.equals("SLEEP")) {
            if (call.args.size() != 1) throw webstrada::exception("Sleep requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfml::cf_sleep(&a1);
            return cfvariant();
        }

        if (fname.equals("BITAND")) {
            if (call.args.size() != 2) throw webstrada::exception("BitAnd requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitand(&a1, &a2));
        }
        if (fname.equals("BITMASKCLEAR")) {
            if (call.args.size() != 3) throw webstrada::exception("BitMaskClear requires exactly 3 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitmaskclear(&a1, &a2, &a3));
        }
        if (fname.equals("BITMASKREAD")) {
            if (call.args.size() != 3) throw webstrada::exception("BitMaskRead requires exactly 3 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitmaskread(&a1, &a2, &a3));
        }
        if (fname.equals("BITMASKSET")) {
            if (call.args.size() != 4) throw webstrada::exception("BitMaskSet requires exactly 4 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a4 = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitmaskset(&a1, &a2, &a3, &a4));
        }
        if (fname.equals("BITNOT")) {
            if (call.args.size() != 1) throw webstrada::exception("BitNot requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitnot(&a1));
        }
        if (fname.equals("BITOR")) {
            if (call.args.size() != 2) throw webstrada::exception("BitOr requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitor(&a1, &a2));
        }
        if (fname.equals("BITSHLN")) {
            if (call.args.size() != 2) throw webstrada::exception("BitSHLN requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitshln(&a1, &a2));
        }
        if (fname.equals("BITSHRN")) {
            if (call.args.size() != 2) throw webstrada::exception("BitSHRN requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitshrn(&a1, &a2));
        }
        if (fname.equals("BITXOR")) {
            if (call.args.size() != 2) throw webstrada::exception("BitXor requires exactly 2 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_bitxor(&a1, &a2));
        }
        if (fname.equals("FINDONEOF")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("FindOneOf requires 2 or 3 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3;
            if (call.args.size() >= 3) a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_findoneof(&a1, &a2, call.args.size() >= 3 ? &a3 : nullptr));
        }
        if (fname.equals("FIX")) {
            if (call.args.size() != 1) throw webstrada::exception("Fix requires exactly 1 argument");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_fix(&a1));
        }
        if (fname.equals("INSERT")) {
            if (call.args.size() != 3) throw webstrada::exception("Insert requires exactly 3 arguments");
            cfvariant a1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant a3 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_insert(&a1, &a2, &a3));
        }

        if (fname.equals("FLOOR")) {
            if (call.args.size() != 1) throw webstrada::exception("Floor requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_floor(&arg));
        }

        if (fname.equals("INCREMENTVALUE")) {
            if (call.args.size() != 1) throw webstrada::exception("IncrementValue requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_incrementvalue(&arg));
        }

        if (fname.equals("DECREMENTVALUE")) {
            if (call.args.size() != 1) throw webstrada::exception("DecrementValue requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_decrementvalue(&arg));
        }

        if (fname.equals("INT")) {
            if (call.args.size() != 1) throw webstrada::exception("Int requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_int(&arg));
        }

        if (fname.equals("LOG")) {
            if (call.args.size() != 1) throw webstrada::exception("Log requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_log(&arg));
        }

        if (fname.equals("LOG10")) {
            if (call.args.size() != 1) throw webstrada::exception("Log10 requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_log10(&arg));
        }

        if (fname.equals("MAX")) {
            if (call.args.size() != 2) throw webstrada::exception("Max requires exactly 2 arguments");
            cfvariant arg1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_max(&arg1, &arg2));
        }

        if (fname.equals("MIN")) {
            if (call.args.size() != 2) throw webstrada::exception("Min requires exactly 2 arguments");
            cfvariant arg1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_min(&arg1, &arg2));
        }

        if (fname.equals("PI")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("Pi requires 0 arguments");
            }
            return tempReturn(cfml::cf_pi());
        }

        if (fname.equals("RAND")) {
            if (call.args.size() > 1) {
                throw webstrada::exception("Rand requires 0 or 1 arguments");
            }
            cfvariant alg(cfvariant::Null);
            if (call.args.size() == 1 && !call.args[0].trimmed().isEmpty())
                alg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_rand(&alg));
        }

        if (fname.equals("RANDOMIZE")) {
            if (call.args.size() < 1 || call.args.size() > 2)
                throw webstrada::exception("Randomize requires 1 or 2 arguments");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant alg(cfvariant::Null);
            if (call.args.size() == 2)
                alg = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_randomize(&arg, &alg));
        }

        if (fname.equals("RANDRANGE")) {
            if (call.args.size() < 2 || call.args.size() > 3)
                throw webstrada::exception("RandRange requires exactly 2 or 3 arguments");
            cfvariant n1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant n2 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant alg(cfvariant::Null);
            if (call.args.size() == 3)
                alg = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_randrange(&n1, &n2, &alg));
        }

        if (fname.equals("ROUND")) {
            if (call.args.size() != 1) throw webstrada::exception("Round requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_round(&arg));
        }

        if (fname.equals("SGN")) {
            if (call.args.size() != 1) throw webstrada::exception("Sgn requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_sgn(&arg));
        }

        if (fname.equals("SIN")) {
            if (call.args.size() != 1) throw webstrada::exception("Sin requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_sin(&arg));
        }

        if (fname.equals("SQR")) {
            if (call.args.size() != 1) throw webstrada::exception("Sqr requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_sqr(&arg));
        }

        if (fname.equals("TAN")) {
            if (call.args.size() != 1) throw webstrada::exception("Tan requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_tan(&arg));
        }

        if (fname.equals("LEN")) {
            if (call.args.size() != 1) throw webstrada::exception("Len requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_len(&arg));
        }

        if (fname.equals("LEFT")) {
            if (call.args.size() != 2) throw webstrada::exception("Left requires exactly 2 arguments");
            cfvariant strVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant cntVal = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_left(&strVal, &cntVal));
        }

        if (fname.equals("RIGHT")) {
            if (call.args.size() != 2) throw webstrada::exception("Right requires exactly 2 arguments");
            cfvariant strVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant cntVal = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_right(&strVal, &cntVal));
        }

        if (fname.equals("MID")) {
            if (call.args.size() != 3) throw webstrada::exception("Mid requires exactly 3 arguments");
            cfvariant strVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant startVal = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant cntVal = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_mid(&strVal, &startVal, &cntVal));
        }

        if (fname.equals("TRIM")) {
            if (call.args.size() != 1) throw webstrada::exception("Trim requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_trim(&arg));
        }

        if (fname.equals("LTRIM")) {
            if (call.args.size() != 1) throw webstrada::exception("LTrim requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_ltrim(&arg));
        }

        if (fname.equals("RTRIM")) {
            if (call.args.size() != 1) throw webstrada::exception("RTrim requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_rtrim(&arg));
        }

        if (fname.equals("LCASE")) {
            if (call.args.size() != 1) throw webstrada::exception("LCase requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_lcase(&arg));
        }

        if (fname.equals("UCASE")) {
            if (call.args.size() != 1) throw webstrada::exception("UCase requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_ucase(&arg));
        }

        if (fname.equals("REVERSE")) {
            if (call.args.size() != 1) throw webstrada::exception("Reverse requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_reverse(&arg));
        }

        if (fname.equals("REPEATSTRING")) {
            if (call.args.size() != 2) throw webstrada::exception("RepeatString requires exactly 2 arguments");
            cfvariant strVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant cntVal = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_repeatstring(&strVal, &cntVal));
        }

        if (fname.equals("REPLACE")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception("Replace requires 3 or 4 arguments");
            cfvariant strVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant sub1Val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant sub2Val = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *scopeVal = nullptr;
            cfvariant tmpScope;
            if (call.args.size() == 4) {
                tmpScope = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
                scopeVal = &tmpScope;
            }
            return tempReturn(cfml::cf_replace(&strVal, &sub1Val, &sub2Val, scopeVal));
        }

        if (fname.equals("REPLACENOCASE")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception("ReplaceNoCase requires 3 or 4 arguments");
            cfvariant strVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant sub1Val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant sub2Val = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *scopeVal = nullptr;
            cfvariant tmpScope;
            if (call.args.size() == 4) {
                tmpScope = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
                scopeVal = &tmpScope;
            }
            return tempReturn(cfml::cf_replacenocase(&strVal, &sub1Val, &sub2Val, scopeVal));
        }

        if (fname.equals("FIND")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("Find requires 2 or 3 arguments");
            cfvariant subVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant strVal = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *startVal = nullptr;
            cfvariant tmpStart;
            if (call.args.size() == 3) {
                tmpStart = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
                startVal = &tmpStart;
            }
            return tempReturn(cfml::cf_find(&subVal, &strVal, startVal));
        }

        if (fname.equals("FINDNOCASE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("FindNoCase requires 2 or 3 arguments");
            cfvariant subVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant strVal = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *startVal = nullptr;
            cfvariant tmpStart;
            if (call.args.size() == 3) {
                tmpStart = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
                startVal = &tmpStart;
            }
            return tempReturn(cfml::cf_findnocase(&subVal, &strVal, startVal));
        }

        if (fname.equals("COMPARE")) {
            if (call.args.size() != 2) throw webstrada::exception("Compare requires exactly 2 arguments");
            cfvariant s1Val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant s2Val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_compare(&s1Val, &s2Val));
        }

        if (fname.equals("COMPARENOCASE")) {
            if (call.args.size() != 2) throw webstrada::exception("CompareNoCase requires exactly 2 arguments");
            cfvariant s1Val = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant s2Val = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_comparenocase(&s1Val, &s2Val));
        }


        if (fname.equals("STRUCTNEW")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("StructNew requires 0 arguments");
            }
            return tempReturn(cfml::cf_structnew());
        }

        if (fname.equals("STRUCTINSERT")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception("StructInsert requires 3 or 4 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("StructInsert: First argument must be a structure variable");
            cfvariant key = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant allowOverwrite;
            if (call.args.size() == 4) {
                allowOverwrite = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_structinsert(var, &key, &val, call.args.size() == 4 ? &allowOverwrite : nullptr));
        }

        if (fname.equals("STRUCTUPDATE")) {
            if (call.args.size() != 3) throw webstrada::exception("StructUpdate requires exactly 3 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("StructUpdate: First argument must be a structure variable");
            cfvariant key = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant val = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structupdate(var, &key, &val));
        }

        if (fname.equals("ISXML")) {
            if (call.args.size() != 1) throw webstrada::exception("IsXML requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isxml(&arg));
        }

        if (fname.equals("ISXMLATTRIBUTE")) {
            if (call.args.size() != 1) throw webstrada::exception("IsXmlAttribute requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isxmlattribute(&arg));
        }

        if (fname.equals("ISXMLDOC")) {
            if (call.args.size() != 1) throw webstrada::exception("IsXmlDoc requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isxmldoc(&arg));
        }

        if (fname.equals("ISXMLELEM")) {
            if (call.args.size() != 1) throw webstrada::exception("IsXmlElem requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isxmlelem(&arg));
        }

        if (fname.equals("ISXMLNODE")) {
            if (call.args.size() != 1) throw webstrada::exception("IsXmlNode requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isxmlnode(&arg));
        }

        if (fname.equals("ISXMLROOT")) {
            if (call.args.size() != 1) throw webstrada::exception("IsXmlRoot requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_isxmlroot(&arg));
        }

        if (fname.equals("SERIALIZEXML")) {
            if (call.args.size() != 1) throw webstrada::exception("SerializeXML requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_serializexml(&arg));
        }

        if (fname.equals("XMLGETNODETYPE")) {
            if (call.args.size() != 1) throw webstrada::exception("XmlGetNodeType requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_xmlgetnodetype(&arg));
        }

        if (fname.equals("XMLNEW")) {
            if (call.args.size() > 1) throw webstrada::exception("XmlNew requires at most 1 argument");
            cfvariant arg;
            if (call.args.size() == 1) {
                arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_xmlnew(call.args.size() == 1 ? &arg : nullptr));
        }

        if (fname.equals("XMLFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("XmlFormat requires 1 or 2 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1;
            if (call.args.size() == 2) {
                arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_xmlformat(&arg0, call.args.size() == 2 ? &arg1 : nullptr));
        }

        if (fname.equals("XMLVALIDATE")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("XmlValidate requires 1 or 2 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1;
            if (call.args.size() == 2) {
                arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_xmlvalidate(&arg0, call.args.size() == 2 ? &arg1 : nullptr));
        }

        if (fname.equals("XMLELEMNEW")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("XmlElemNew requires 2 or 3 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg2;
            if (call.args.size() == 3) {
                arg2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_xmlelemnew(&arg0, &arg1, call.args.size() == 3 ? &arg2 : nullptr));
        }

        if (fname.equals("XMLSEARCH")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("XmlSearch requires 2 or 3 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg2;
            if (call.args.size() == 3) {
                arg2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_xmlsearch(&arg0, &arg1, call.args.size() == 3 ? &arg2 : nullptr));
        }

        if (fname.equals("XMLTRANSFORM")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("XmlTransform requires 2 or 3 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg2;
            if (call.args.size() == 3) {
                arg2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_xmltransform(&arg0, &arg1, call.args.size() == 3 ? &arg2 : nullptr));
        }

        if (fname.equals("XMLPARSE")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("XmlParse requires between 1 and 3 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1;
            if (call.args.size() >= 2) {
                arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            cfvariant arg2;
            if (call.args.size() == 3) {
                arg2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_xmlparse(&arg0, call.args.size() >= 2 ? &arg1 : nullptr, call.args.size() == 3 ? &arg2 : nullptr));
        }

        if (fname.equals("DESERIALIZEXML")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("DeserializeXML requires between 1 and 3 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1;
            if (call.args.size() >= 2) {
                arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            cfvariant arg2;
            if (call.args.size() == 3) {
                arg2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_deserializexml(&arg0, call.args.size() >= 2 ? &arg1 : nullptr, call.args.size() == 3 ? &arg2 : nullptr));
        }

        if (fname.equals("XMLCHILDPOS")) {
            if (call.args.size() != 3) throw webstrada::exception("XmlChildPos requires exactly 3 arguments");
            cfvariant arg0 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg1 = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant arg2 = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_xmlchildpos(&arg0, &arg1, &arg2));
        }

        if (fname.equals("STRUCTDELETE")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("StructDelete requires 2 or 3 arguments");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("StructDelete: First argument must be a structure variable");
            cfvariant key = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant indicateExisting;
            if (call.args.size() == 3) {
                indicateExisting = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_structdelete(var, &key, call.args.size() == 3 ? &indicateExisting : nullptr));
        }

        if (fname.equals("STRUCTFIND")) {
            if (call.args.size() != 2) throw webstrada::exception("StructFind requires exactly 2 arguments");
            cfvariant arg1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant key = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structfind(&arg1, &key));
        }

        if (fname.equals("STRUCTKEYEXISTS")) {
            if (call.args.size() != 2) throw webstrada::exception("StructKeyExists requires exactly 2 arguments");
            cfvariant arg1 = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant key = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structkeyexists(&arg1, &key));
        }

        if (fname.equals("STRUCTISEMPTY")) {
            if (call.args.size() != 1) throw webstrada::exception("StructIsEmpty requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structisempty(&arg));
        }

        if (fname.equals("STRUCTCOUNT")) {
            if (call.args.size() != 1) throw webstrada::exception("StructCount requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structcount(&arg));
        }

        if (fname.equals("STRUCTCLEAR")) {
            if (call.args.size() != 1) throw webstrada::exception("StructClear requires exactly 1 argument");
            cfvariant *var = lookupVarWritable(call.args[0].constData(), cgi, server, cookie, application, session, url, form, variables);
            if (!var) throw webstrada::exception("StructClear: Argument must be a structure variable");
            return tempReturn(cfml::cf_structclear(var));
        }

        if (fname.equals("STRUCTKEYARRAY")) {
            if (call.args.size() != 1) throw webstrada::exception("StructKeyArray requires exactly 1 argument");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_structkeyarray(&arg));
        }

        if (fname.equals("STRUCTKEYLIST")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("StructKeyList requires 1 or 2 arguments");
            cfvariant arg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant delim;
            if (call.args.size() == 2) {
                delim = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_structkeylist(&arg, call.args.size() == 2 ? &delim : nullptr));
        }

        if (fname.equals("CREATEGUID") || fname.equals("CREATEUUID")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("CreateGUID/CreateUUID requires 0 arguments");
            }
            // CF's CreateUUID format is 8-4-4-16 (35 chars: 32 hex + 3 dashes),
            // not the standard 8-4-4-4-12 UUID — verified on the RDS host
            // (e.g. C6B34173-E479-FF88-D83FCE70D36BCF12); IsValid("uuid")
            // matches this shape. CreateGUID was removed in CF 2025.
            char buf[36];
            std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%016llx",
                          rand(), rand() & 0xffff, (rand() & 0xffff),
                          (static_cast<unsigned long long>(rand()) << 32) |
                          ((static_cast<unsigned long long>(rand()) << 16) & 0xffffffffffffULL) |
                          (rand() & 0xffff));
            return cfvariant(buf);
        }

        if (fname.equals("GETBASETEMPLATEPATH")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("GetBaseTemplatePath requires 0 arguments");
            }
            IncludeRuntime *rt = cfml::include_context();
            if (rt && !rt->currentPath.empty()) {
                return cfvariant(std::filesystem::absolute(rt->currentPath).string().c_str());
            }
            return cfvariant(std::filesystem::absolute("index.cfm").string().c_str());
        }

        if (fname.equals("GETCURRENTTEMPLATEPATH")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("GetCurrentTemplatePath requires 0 arguments");
            }
            IncludeRuntime *rt = cfml::include_context();
            if (rt && !rt->currentPath.empty()) {
                return cfvariant(std::filesystem::absolute(rt->currentPath).string().c_str());
            }
            return cfvariant(std::filesystem::absolute("index.cfm").string().c_str());
        }

        if (fname.equals("GETDIRECTORYFROMPATH")) {
            if (call.args.size() != 1) throw webstrada::exception("GetDirectoryFromPath requires exactly 1 argument");
            cfvariant pathVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            std::filesystem::path p(pathVal.toString().constData());
            std::filesystem::path dir = p.parent_path();
            string dirStr = dir.string().c_str();
            if (!dirStr.isEmpty() && dirStr.at(dirStr.length() - 1) != '/') {
                dirStr += "/";
            }
            return cfvariant(dirStr);
        }

        if (fname.equals("GETFILEFROMPATH")) {
            if (call.args.size() != 1) throw webstrada::exception("GetFileFromPath requires exactly 1 argument");
            cfvariant pathVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            std::filesystem::path p(pathVal.toString().constData());
            return cfvariant(p.filename().string().c_str());
        }

        if (fname.equals("GETLOCALE")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("GetLocale requires 0 arguments");
            }
            return tempReturn(cfml::cf_getlocale());
        }

        if (fname.equals("SETLOCALE")) {
            if (call.args.size() != 1) throw webstrada::exception("SetLocale requires exactly 1 argument");
            cfvariant loc = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cfml::cf_setlocale(&loc));
        }

        if (fname.equals("LSCURRENCYFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("LSCurrencyFormat requires between 1 and 3 arguments");
            cfvariant num = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant typ = (call.args.size() >= 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            cfvariant loc = (call.args.size() == 3) ? evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lscurrencyformat(&num, &typ, &loc));
        }

        if (fname.equals("LSEUROCURRENCYFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("LSEuroCurrencyFormat requires between 1 and 3 arguments");
            cfvariant num = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant typ = (call.args.size() >= 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            cfvariant loc = (call.args.size() == 3) ? evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lseurocurrencyformat(&num, &typ, &loc));
        }

        if (fname.equals("LSISCURRENCY")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("LSIsCurrency requires 1 or 2 arguments");
            cfvariant str = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant loc = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsiscurrency(&str, &loc));
        }

        if (fname.equals("LSNUMBERFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("LSNumberFormat requires between 1 and 3 arguments");
            cfvariant num = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant msk = (call.args.size() >= 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            cfvariant loc = (call.args.size() == 3) ? evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsnumberformat(&num, &msk, &loc));
        }

        if (fname.equals("LSPARSECURRENCY")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("LSParseCurrency requires 1 or 2 arguments");
            cfvariant str = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant loc = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsparsecurrency(&str, &loc));
        }

        if (fname.equals("LSPARSEEUROCURRENCY")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("LSParseEuroCurrency requires 1 or 2 arguments");
            cfvariant str = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant loc = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsparseeurocurrency(&str, &loc));
        }

        if (fname.equals("LSPARSENUMBER")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("LSParseNumber requires 1 or 2 arguments");
            cfvariant str = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant loc = (call.args.size() == 2) ? evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables) : cfvariant(cfvariant::Null);
            return tempReturn(cfml::cf_lsparsenumber(&str, &loc));
        }

        if (fname.equals("GETTICKCOUNT")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("GetTickCount requires 0 arguments");
            }
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            cfvariant res(cfvariant::Float);
            res.m_double = static_cast<double>(ms);
            return res;
        }

        if (fname.equals("GETTEMPDIRECTORY")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception("GetTempDirectory requires 0 arguments");
            }
            std::filesystem::path tmp = std::filesystem::temp_directory_path();
            string tmpStr = tmp.string().c_str();
            if (!tmpStr.isEmpty() && tmpStr.at(tmpStr.length() - 1) != '/') {
                tmpStr += "/";
            }
            return cfvariant(tmpStr);
        }

        if (fname.equals("GETTEMPFILE")) {
            if (call.args.size() != 2) throw webstrada::exception("GetTempFile requires exactly 2 arguments");
            cfvariant dirVal = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant prefixVal = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            std::filesystem::path dpath(dirVal.toString().constData());
            string prefix = prefixVal.toString();
            std::filesystem::path fpath;
            do {
                char tmpName[64];
                std::snprintf(tmpName, sizeof(tmpName), "%s%08x.tmp", prefix.constData(), rand());
                fpath = dpath / tmpName;
            } while (std::filesystem::exists(fpath));
            std::ofstream outfile(fpath);
            outfile.close();
            return cfvariant(fpath.string().c_str());
        }

        if (fname.equals("WRITEDUMP")) {
            if (call.args.size() < 1 || call.args.size() > 12) throw webstrada::exception("WriteDump requires 1 to 12 arguments");
            cfvariant args[12];
            for (size_t i = 0; i < call.args.size(); i++) {
                args[i] = evaluateExpr(out, call.args[i], cgi, server, cookie, application, session, url, form, variables);
            }
            cfvariant *dump = cfml::cf_writedump(&args[0], call.args.size() > 1 ? &args[1] : nullptr,
                call.args.size() > 2 ? &args[2] : nullptr, call.args.size() > 3 ? &args[3] : nullptr,
                call.args.size() > 4 ? &args[4] : nullptr, call.args.size() > 5 ? &args[5] : nullptr,
                call.args.size() > 6 ? &args[6] : nullptr, call.args.size() > 7 ? &args[7] : nullptr,
                call.args.size() > 8 ? &args[8] : nullptr, call.args.size() > 9 ? &args[9] : nullptr,
                call.args.size() > 10 ? &args[10] : nullptr, call.args.size() > 11 ? &args[11] : nullptr);
            cfml::cf_emit_writedump(&out, dump);
            return cfvariant("");
        }

        if (fname.equals("WRITEOUTPUT")) {
            if (cfml::response().binary) return cfvariant(""); // cfcontent file/variable: other output ignored
            if (call.args.size() < 1 || (call.args.size() == 1 && call.args[0].isEmpty()))
                throw webstrada::exception("WriteOutput requires at least 1 argument");
            for (size_t i = 0; i < call.args.size(); i++) {
                cfvariant arg = evaluateExpr(out, call.args[i], cgi, server, cookie, application, session, url, form, variables);
                switch (arg.m_type) {
                case cfvariant::String:
                    out.append(*arg.m_str);
                    break;
                case cfvariant::Number:
                    out.append(string::number(arg.m_int));
                    break;
                case cfvariant::Long:
                    out.append(string::number(arg.m_long));
                    break;
                case cfvariant::Float: {
                    if (arg.m_literalText) {
                        out.append(*arg.m_literalText);
                    } else {
                        std::string s = formatCfdumpFloat(arg.m_double);
                        out.append(s.c_str());
                    }
                    break;
                }
                case cfvariant::Boolean:
                    if (arg.m_boolLiteral) {
                        out.append(arg.m_bool ? "true" : "false");
                    } else {
                        out.append(arg.m_bool ? "YES" : "NO");
                    }
                    break;
                case cfvariant::Null:
                    out.append("[null]");
                    break;
                case cfvariant::Function:
                    // A bare built-in function reference (writeOutput(pi)) is a
                    // method handle that renders its text like CF 2021.
                    out.append(*arg.m_str);
                    break;
                default:
                    break;
                }
            }
            cfvariant res(cfvariant::Boolean);
            res.m_bool = true;
            return res;
        }

        // JSON functions
        if (fname.equals("CANSERIALIZE") || fname.equals("CANDESERIALIZE")) {
            cfvariant res(cfvariant::Boolean);
            res.m_bool = true;
            return res;
        }

        if (fname.equals("SERIALIZEJSON") || fname.equals("SERIALIZE")) {
            if (call.args.size() < 1) throw webstrada::exception(fname + " requires at least 1 argument");
            cfvariant data = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("SERIALIZE")) {
                if (call.args.size() < 2) throw webstrada::exception("Serialize requires at least 2 arguments");
                cfvariant typeArg = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
                return tempReturn(cfml::cf_serialize(&data, &typeArg));
            }
            cfvariant qf;
            if (call.args.size() >= 2) {
                qf = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_serializejson(&data, call.args.size() >= 2 ? &qf : nullptr));
        }

        if (fname.equals("DESERIALIZEJSON") || fname.equals("DESERIALIZE")) {
            if (call.args.size() < 1) throw webstrada::exception(fname + " requires at least 1 argument");
            cfvariant jsonArg = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("DESERIALIZE")) {
                if (call.args.size() < 2) throw webstrada::exception("Deserialize requires at least 2 arguments");
                cfvariant typeArg = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
                return tempReturn(cfml::cf_deserialize(&jsonArg, &typeArg));
            }
            // strictMapping is the (boolean) 2nd argument of DeserializeJSON.
            cfvariant sm;
            if (call.args.size() >= 2) {
                sm = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cfml::cf_deserializejson(&jsonArg, call.args.size() >= 2 ? &sm : nullptr));
        }

        if (fname.equals("GETCOMPONENTMETADATA")) {
            if (call.args.size() != 1) throw webstrada::exception("GetComponentMetaData requires exactly 1 argument");
            cfvariant v = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_getcomponentmetadata_impl(&v));
        }

        if (fname.equals("ISINSTANCEOF")) {
            if (call.args.size() != 2) throw webstrada::exception("IsInstanceOf requires exactly 2 arguments");
            cfvariant obj = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant t = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_isinstanceof_impl(&obj, &t));
        }

        // ---- Tier-1 built-in functions (see CFFUNCTION_IMPLEMENTATION_ACTION_PLAN.md) ----

        // No-argument functions.
        if (fname.equals("GETCONTEXTROOT") || fname.equals("GETLOCALHOSTIP") || fname.equals("ISDEBUGMODE") ||
            fname.equals("GETSYSTEMFREEMEMORY") || fname.equals("GETSYSTEMTOTALMEMORY") ||
            fname.equals("GETFUNCTIONLIST") || fname.equals("GETCSPNONCE") || fname.equals("GETCLIENTVARIABLESLIST") ||
            fname.equals("TRANSACTIONCOMMIT")) {
            if (call.args.size() > 0 && !(call.args.size() == 1 && call.args[0].isEmpty())) {
                throw webstrada::exception(fname + " requires 0 arguments");
            }
            if (fname.equals("GETCONTEXTROOT")) return tempReturn(cf_getcontextroot());
            if (fname.equals("GETLOCALHOSTIP")) return tempReturn(cf_getlocalhostip());
            if (fname.equals("ISDEBUGMODE")) return tempReturn(cf_isdebugmode());
            if (fname.equals("GETSYSTEMFREEMEMORY")) return tempReturn(cf_getsystemfreememory());
            if (fname.equals("GETSYSTEMTOTALMEMORY")) return tempReturn(cf_getsystemtotalmemory());
            if (fname.equals("GETFUNCTIONLIST")) return tempReturn(cf_getfunctionlist());
            if (fname.equals("GETCSPNONCE")) return tempReturn(cf_getcspnonce());
            if (fname.equals("GETCLIENTVARIABLESLIST")) return tempReturn(cf_getclientvariableslist());
            return tempReturn(cf_transactioncommit());
        }

        // Single-argument functions.
        if (fname.equals("GETENCODING") || fname.equals("GETFREESPACE") || fname.equals("GETTOTALSPACE") ||
            fname.equals("ISIPV6") || fname.equals("ISLOCALHOST") || fname.equals("GETMETRICDATA") ||
            fname.equals("ISDDX") || fname.equals("ISWDDX") || fname.equals("DELETECLIENTVARIABLE") ||
            fname.equals("PRESERVESINGLEQUOTES") || fname.equals("CREATEODBCDATE") ||
            fname.equals("CREATEODBCTIME") || fname.equals("ISTHREADINTERRUPTED")) {
            if (call.args.size() != 1) throw webstrada::exception(fname + " requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("GETENCODING")) return tempReturn(cf_getencoding(&a));
            if (fname.equals("GETFREESPACE")) return tempReturn(cf_getfreespace(&a));
            if (fname.equals("GETTOTALSPACE")) return tempReturn(cf_gettotalspace(&a));
            if (fname.equals("ISIPV6")) return tempReturn(cf_isipv6(&a));
            if (fname.equals("ISLOCALHOST")) return tempReturn(cf_islocalhost(&a));
            if (fname.equals("GETMETRICDATA")) return tempReturn(cf_getmetricdata(&a));
            if (fname.equals("ISDDX")) return tempReturn(cf_isddx(&a));
            if (fname.equals("ISWDDX")) return tempReturn(cf_iswddx(&a));
            if (fname.equals("DELETECLIENTVARIABLE")) return tempReturn(cf_deleteclientvariable(&a));
            if (fname.equals("PRESERVESINGLEQUOTES")) return tempReturn(cf_preservesinglequotes(&a));
            if (fname.equals("CREATEODBCDATE")) return tempReturn(cf_createodbcdate(&a));
            if (fname.equals("CREATEODBCTIME")) return tempReturn(cf_createodbctime(&a));
            return tempReturn(cf_isthreadinterrupted(&a));
        }

        if (fname.equals("OBJECTEQUALS")) {
            if (call.args.size() != 2) throw webstrada::exception("ObjectEquals requires exactly 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_objectequals(&a, &b));
        }

        if (fname.equals("GETTOKEN")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("GetToken requires 2 or 3 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c;
            if (call.args.size() == 3) {
                c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cf_gettoken(&a, &b, call.args.size() == 3 ? &c : nullptr));
        }

        if (fname.equals("TRANSACTIONROLLBACK")) {
            if (call.args.size() > 1) throw webstrada::exception("TransactionRollback requires 0 or 1 arguments");
            cfvariant a;
            if (call.args.size() == 1) {
                a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cf_transactionrollback(call.args.size() == 1 ? &a : nullptr));
        }

        if (fname.equals("TRANSACTIONSETSAVEPOINT")) {
            if (call.args.size() != 1) throw webstrada::exception("TransactionSetSavePoint requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_transactionsetsavepoint(&a));
        }

        if (fname.equals("LOCATION")) {
            cfvariant a, b, c;
            if (call.args.size() >= 1) a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_location(call.args.size() >= 1 ? &a : nullptr,
                                call.args.size() >= 2 ? &b : nullptr,
                                call.args.size() >= 3 ? &c : nullptr,
                                static_cast<int>(call.args.size())));
        }

        if (fname.equals("SETVARIABLE")) {
            if (call.args.size() != 2) throw webstrada::exception("SetVariable requires exactly 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_setvariable(&a, &b, cgi, server, cookie, application, session, url, form, variables));
        }

        // ---- Tier-2 encoder family (CFFUNCTION_IMPLEMENTATION_ACTION_PLAN.md) ----
        if (fname.equals("ENCODEFORHTML") || fname.equals("ENCODEFORHTMLATTRIBUTE") || fname.equals("ENCODEFORJAVASCRIPT") ||
            fname.equals("ENCODEFORCSS") || fname.equals("ENCODEFORXML") || fname.equals("ENCODEFORXMLATTRIBUTE") ||
            fname.equals("ENCODEFORDN") || fname.equals("ENCODEFORLDAP") || fname.equals("ENCODEFORXPATH")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception(fname + " requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) {
                b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            const cfvariant *pb = call.args.size() == 2 ? &b : nullptr;
            if (fname.equals("ENCODEFORHTML")) return tempReturn(cf_encodeforhtml(&a, pb));
            if (fname.equals("ENCODEFORHTMLATTRIBUTE")) return tempReturn(cf_encodeforhtmlattribute(&a, pb));
            if (fname.equals("ENCODEFORJAVASCRIPT")) return tempReturn(cf_encodeforjavascript(&a, pb));
            if (fname.equals("ENCODEFORCSS")) return tempReturn(cf_encodeforcss(&a, pb));
            if (fname.equals("ENCODEFORXML")) return tempReturn(cf_encodeforxml(&a, pb));
            if (fname.equals("ENCODEFORXMLATTRIBUTE")) return tempReturn(cf_encodeforxmlattribute(&a, pb));
            if (fname.equals("ENCODEFORDN")) return tempReturn(cf_encodefordn(&a, pb));
            if (fname.equals("ENCODEFORLDAP")) return tempReturn(cf_encodeforldap(&a, pb));
            return tempReturn(cf_encodeforxpath(&a, pb));
        }

        if (fname.equals("DECODEFORHTML")) {
            if (call.args.size() != 1) throw webstrada::exception("DecodeForHTML requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_decodeforhtml(&a));
        }

        if (fname.equals("CANONICALIZE")) {
            if (call.args.size() < 3 || call.args.size() > 4) throw webstrada::exception("Canonicalize requires 3 or 4 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant d;
            if (call.args.size() == 4) {
                d = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cf_canonicalize(&a, &b, &c, call.args.size() == 4 ? &d : nullptr));
        }

        if (fname.equals("NUMBERFORMAT")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("NumberFormat requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) {
                b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            }
            return tempReturn(cf_numberformat(&a, call.args.size() == 2 ? &b : nullptr));
        }

        if (fname.equals("DUPLICATE")) {
            if (call.args.size() != 1) throw webstrada::exception("Duplicate requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_duplicate(&a));
        }

        if (fname.equals("IIF")) {
            if (call.args.size() != 3) throw webstrada::exception("IIf requires exactly 3 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_iif(&a, &b, &c, out, cgi, server, cookie, application, session, url, form, variables));
        }

        if (fname.equals("ISVALID")) {
            if (call.args.size() < 2 || call.args.size() > 5) throw webstrada::exception("IsValid requires 2 to 5 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c, d, e;
            if (call.args.size() >= 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) d = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 5) e = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_isvalid(&a, &b,
                               call.args.size() >= 3 ? &c : nullptr,
                               call.args.size() >= 4 ? &d : nullptr,
                               call.args.size() == 5 ? &e : nullptr));
        }

        if (fname.equals("GETEXCEPTION")) {
            if (call.args.size() != 1) throw webstrada::exception("GetException requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_getexception(&a));
        }

        if (fname.equals("GETLOCALEDISPLAYNAME")) {
            if (call.args.size() > 2) throw webstrada::exception("GetLocaleDisplayName requires 0 to 2 arguments");
            cfvariant a, b;
            if (call.args.size() >= 1) a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_getlocaledisplayname(call.args.size() >= 1 ? &a : nullptr,
                                            call.args.size() == 2 ? &b : nullptr));
        }

        if (fname.equals("GETCPUUSAGE")) {
            if (call.args.size() > 1) throw webstrada::exception("GetCPUUsage requires 0 or 1 arguments");
            cfvariant a;
            if (call.args.size() == 1) a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_getcpuusage(call.args.size() == 1 ? &a : nullptr));
        }

        if (fname.equals("GETMETADATA")) {
            if (call.args.size() != 1) throw webstrada::exception("GetMetaData requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_getmetadata(&a));
        }

        if (fname.equals("SETPROFILESTRING")) {
            if (call.args.size() < 4 || call.args.size() > 5) throw webstrada::exception("SetProfileString requires 4 or 5 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            cfvariant d = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            cfvariant e;
            if (call.args.size() == 5) e = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_setprofilestring(&a, &b, &c, &d, call.args.size() == 5 ? &e : nullptr));
        }

        if (fname.equals("GETPROPERTYSTRING")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("GetPropertyString requires 2 or 3 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c;
            if (call.args.size() == 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_getpropertystring(&a, &b, call.args.size() == 3 ? &c : nullptr));
        }

        if (fname.equals("GETPROPERTYFILE")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("GetPropertyFile requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_getpropertyfile(&a, call.args.size() == 2 ? &b : nullptr));
        }

        if (fname.equals("SETPROPERTYSTRING")) {
            if (call.args.size() < 2 || call.args.size() > 4) throw webstrada::exception("SetPropertyString requires 2 to 4 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c, d;
            if (call.args.size() >= 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) d = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_setpropertystring(&a, &b,
                                         call.args.size() >= 3 ? &c : nullptr,
                                         call.args.size() == 4 ? &d : nullptr));
        }

        if (fname.equals("CSRFGENERATETOKEN")) {
            if (call.args.size() > 2) throw webstrada::exception("CSRFGenerateToken requires 0 to 2 arguments");
            cfvariant a, b;
            if (call.args.size() >= 1) a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_csrfgeneratetoken(call.args.size() >= 1 ? &a : nullptr,
                                         call.args.size() == 2 ? &b : nullptr));
        }

        if (fname.equals("CSRFVERIFYTOKEN")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("CSRFVerifyToken requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_csrfverifytoken(&a, call.args.size() == 2 ? &b : nullptr));
        }

        if (fname.equals("ISONLINE")) {
            if (call.args.size() != 1) throw webstrada::exception("isOnline requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_isonline(&a));
        }

        if (fname.equals("ISPDFFILE") || fname.equals("ISPDFOBJECT")) {
            if (call.args.size() != 1) throw webstrada::exception(fname + " requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return fname.equals("ISPDFFILE") ? *cf_ispdffile(&a) : *cf_ispdfobject(&a);
        }

        if (fname.equals("ISPDFARCHIVE")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("IsPDFArchive requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_ispdfarchive(&a, call.args.size() == 2 ? &b : nullptr));
        }

        if (fname.equals("CSVREAD")) {
            if (call.args.size() < 1 || call.args.size() > 4) throw webstrada::exception("CSVRead requires 1 to 4 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b, c, d;
            if (call.args.size() >= 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) d = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_csvread(&a,
                               call.args.size() >= 2 ? &b : nullptr,
                               call.args.size() >= 3 ? &c : nullptr,
                               call.args.size() == 4 ? &d : nullptr));
        }

        if (fname.equals("CSVWRITE")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("CSVWrite requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_csvwrite(&a, call.args.size() == 2 ? &b : nullptr));
        }

        if (fname.equals("CSVPROCESS")) {
            if (call.args.size() < 2 || call.args.size() > 3) throw webstrada::exception("CSVProcess requires 2 or 3 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c;
            if (call.args.size() == 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_csvprocess(&a, &b, call.args.size() == 3 ? &c : nullptr,
                                  out, cgi, server, cookie, application, session, url, form, variables));
        }

        if (fname.equals("OBJECTSAVE")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("ObjectSave requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_objectsave(&a, call.args.size() == 2 ? &b : nullptr));
        }

        if (fname.equals("OBJECTLOAD")) {
            if (call.args.size() != 1) throw webstrada::exception("ObjectLoad requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_objectload(&a));
        }

        // ---- Cache family (sqlite-backed) ----
        if (fname.equals("CACHEGET")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("CacheGet requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cacheget(&a, call.args.size() == 2 ? &b : nullptr));
        }
        if (fname.equals("CACHEPUT")) {
            if (call.args.size() < 2 || call.args.size() > 6) throw webstrada::exception("CachePut requires 2 to 6 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            cfvariant c, d, e, f;
            if (call.args.size() >= 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 4) d = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 5) e = evaluateExpr(out, call.args[4], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 6) f = evaluateExpr(out, call.args[5], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cacheput(&a, &b,
                                call.args.size() >= 3 ? &c : nullptr,
                                call.args.size() >= 4 ? &d : nullptr,
                                call.args.size() >= 5 ? &e : nullptr,
                                call.args.size() == 6 ? &f : nullptr));
        }
        if (fname.equals("CACHEGETALLIDS")) {
            if (call.args.size() > 2) throw webstrada::exception("CacheGetAllIds requires 0 to 2 arguments");
            cfvariant a, b;
            if (call.args.size() >= 1) a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cachegetallids(call.args.size() >= 1 ? &a : nullptr,
                                      call.args.size() == 2 ? &b : nullptr));
        }
        if (fname.equals("CACHEGETMETADATA")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("CacheGetMetadata requires 1 to 3 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b, c;
            if (call.args.size() >= 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cachegetmetadata(&a, call.args.size() >= 2 ? &b : nullptr,
                                        call.args.size() == 3 ? &c : nullptr));
        }
        if (fname.equals("CACHEGETPROPERTIES")) {
            if (call.args.size() > 1) throw webstrada::exception("CacheGetProperties requires 0 or 1 arguments");
            cfvariant a;
            if (call.args.size() == 1) a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cachegetproperties(call.args.size() == 1 ? &a : nullptr));
        }
        if (fname.equals("CACHEGETSESSION")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("CacheGetSession requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cachegetsession(&a, call.args.size() == 2 ? &b : nullptr));
        }
        if (fname.equals("CACHEIDEXISTS")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("CacheIdExists requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cacheidexists(&a, call.args.size() == 2 ? &b : nullptr));
        }
        if (fname.equals("CACHEREGIONEXISTS")) {
            if (call.args.size() != 1) throw webstrada::exception("CacheRegionExists requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cacheregionexists(&a));
        }
        if (fname.equals("CACHEREGIONNEW")) {
            if (call.args.size() < 1 || call.args.size() > 3) throw webstrada::exception("CacheRegionNew requires 1 to 3 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b, c;
            if (call.args.size() >= 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cacheregionnew(&a, call.args.size() >= 2 ? &b : nullptr,
                                      call.args.size() == 3 ? &c : nullptr));
        }
        if (fname.equals("CACHEREGIONREMOVE")) {
            if (call.args.size() != 1) throw webstrada::exception("CacheRegionRemove requires exactly 1 argument");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cacheregionremove(&a));
        }
        if (fname.equals("CACHEREMOVE")) {
            if (call.args.size() < 1 || call.args.size() > 4) throw webstrada::exception("CacheRemove requires 1 to 4 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b, c, d;
            if (call.args.size() >= 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) d = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cacheremove(&a, call.args.size() >= 2 ? &b : nullptr,
                                   call.args.size() >= 3 ? &c : nullptr,
                                   call.args.size() == 4 ? &d : nullptr));
        }
        if (fname.equals("CACHEREMOVEALL")) {
            if (call.args.size() > 1) throw webstrada::exception("CacheRemoveAll requires 0 or 1 arguments");
            cfvariant a;
            if (call.args.size() == 1) a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cacheremoveall(call.args.size() == 1 ? &a : nullptr));
        }
        if (fname.equals("CACHESETPROPERTIES")) {
            if (call.args.size() < 1 || call.args.size() > 2) throw webstrada::exception("CacheSetProperties requires 1 or 2 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b;
            if (call.args.size() == 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_cachesetproperties(&a, call.args.size() == 2 ? &b : nullptr));
        }
        if (fname.equals("REMOVECACHEDQUERY")) {
            if (call.args.size() < 1 || call.args.size() > 4) throw webstrada::exception("RemoveCachedQuery requires 1 to 4 arguments");
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant b, c, d;
            if (call.args.size() >= 2) b = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() >= 3) c = evaluateExpr(out, call.args[2], cgi, server, cookie, application, session, url, form, variables);
            if (call.args.size() == 4) d = evaluateExpr(out, call.args[3], cgi, server, cookie, application, session, url, form, variables);
            return tempReturn(cf_removecachedquery(&a, call.args.size() >= 2 ? &b : nullptr,
                                         call.args.size() >= 3 ? &c : nullptr,
                                         call.args.size() == 4 ? &d : nullptr));
        }

        if (fname.equals("TRACE")) {
            // trace([var], [text], [type], [category], [inline], [abort]) — the
            // <cftrace> tag as a function. Named arguments arrive as the marker
            // struct at args[0]; positional arguments are rejected by CF.
            cfvariant *namedArg = nullptr;
            if (!call.args.empty()) {
                cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
                if (call.args.size() == 1 && a.m_type == cfvariant::Struct && a.m_struct) {
                    namedArg = &a;
                } else {
                    throw webstrada::exception("Attribute validation error for trace.");
                }
            }
            return tempReturn(cf_trace(namedArg));
        }

        if (fname.equals("GETAUTHUSER") || fname.equals("GETUSERROLES") || fname.equals("ISUSERLOGGEDIN")) {
            if (!call.args.empty()) {
                throw webstrada::exception(fname + " does not take any arguments");
            }
            if (fname.equals("GETAUTHUSER")) return tempReturn(cfml::cf_getauthuser());
            if (fname.equals("GETUSERROLES")) return tempReturn(cfml::cf_getuserroles());
            return tempReturn(cfml::cf_isuserloggedin());
        }
        if (fname.equals("ISUSERINROLE") || fname.equals("ISUSERINANYROLE")) {
            if (call.args.size() != 1) {
                throw webstrada::exception(fname + " requires exactly 1 argument");
            }
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            if (fname.equals("ISUSERINROLE")) return tempReturn(cfml::cf_isuserinrole(&a));
            return tempReturn(cfml::cf_isuserinanyrole(&a));
        }
        if (fname.equals("GETBASETAGLIST")) {
            if (!call.args.empty()) {
                throw webstrada::exception("GetBaseTagList does not take any arguments");
            }
            return tempReturn(cfml::cf_getbasetaglist());
        }
        if (fname.equals("GETBASETAGDATA")) {
            if (call.args.size() < 1 || call.args.size() > 2) {
                throw webstrada::exception("GetBaseTagData requires 1 to 2 arguments");
            }
            cfvariant a = evaluateExpr(out, call.args[0], cgi, server, cookie, application, session, url, form, variables);
            cfvariant *level = nullptr;
            cfvariant lv;
            if (call.args.size() == 2) {
                lv = evaluateExpr(out, call.args[1], cgi, server, cookie, application, session, url, form, variables);
                level = &lv;
            }
            return tempReturn(cfml::cf_getbasetagdata(&a, level));
        }

        if (isKnownFunctionName(fname)) {
            throw webstrada::exception("Unknown function call: " + call.name);
        }
        // CF resolves an unknown callee as a variable reference.
        string uname = call.name;
        uname.toUpper();
        throw webstrada::exception(string("Variable ") + uname + " is undefined.");
    }

    // 5. Check for array index access, e.g. myArray[1] or myStruct["key"]
    int lBracket = e.indexOf('[');
    int rBracket = e.indexOf(']');
    if (lBracket > 0 && rBracket > lBracket) {
        string arrayName = e.left(lBracket).trimmed();
        string idxExpr = e.mid(lBracket + 1, rBracket - lBracket - 1).trimmed();
        cfvariant *arrVar = lookupVarWritable(arrayName.constData(), cgi, server, cookie, application, session, url, form, variables);
        if (arrVar && (arrVar->m_type == cfvariant::Array || arrVar->m_type == cfvariant::Struct || arrVar->m_type == cfvariant::Xml || arrVar->m_type == cfvariant::Query || arrVar->m_type == cfvariant::String)) {
            cfvariant idxVal = evaluateExpr(out, idxExpr, cgi, server, cookie, application, session, url, form, variables);
            if (arrVar->m_type == cfvariant::Array) {
                int idx = (idxVal.m_type == cfvariant::Number) ? idxVal.m_int : atoi(idxVal.toString().constData());
                // A live query-column reference reads through to the query's
                // cells; a row past the last one reads as empty (CF 2021).
                if (arrVar->m_queryColOwner && arrVar->m_queryColIndex >= 0) {
                    QueryData *qd = arrVar->m_queryColOwner;
                    int colIdx = arrVar->m_queryColIndex;
                    if (colIdx >= 0 && colIdx < (int)qd->columns.size()) {
                        QueryColumn &col = qd->columns[colIdx];
                        if (idx >= 1 && idx <= (int)col.values.size()) {
                            return col.values[idx - 1];
                        }
                        return cfvariant(cfvariant::Null);
                    }
                }
                if (idx >= 1 && idx <= (int)arrVar->m_array->size()) {
                    return arrVar->m_array->at(idx - 1);
                }
                // CF reports the array variable name for a direct single index
                // (killArray[1]); nested/expression bases use the object form
                // (was BUGS.md "Array index out-of-bounds error message").
                const char *oobName = isSimpleIdentifierName(arrayName) ? arrayName.constData() : nullptr;
                cf_throw_array_oob(idx, 1, oobName);
            }
            if (arrVar->m_type == cfvariant::String) {
                int idx = (idxVal.m_type == cfvariant::Number) ? idxVal.m_int : atoi(idxVal.toString().constData());
                if (idx < 1 || idx > (int)arrVar->m_str->length()) {
                    throw webstrada::exception(string("Cannot access array element at position ") + string::number(idx) + ".");
                }
                cfvariant res(cfvariant::String);
                res.m_str->clear();
                res.m_str->append(arrVar->m_str->at(idx - 1));
                return res;
            }
            if (arrVar->m_type == cfvariant::Query && arrVar->m_query) {
                string key = idxVal.toString();
                // Bracket access resolves columns before pseudo-properties
                // (verified against CF 2021).
                int colIdx = arrVar->m_query->findColumn(key);
                if (colIdx >= 0) {
                    cfvariant colArr(cfvariant::Array);
                    for (auto &v : arrVar->m_query->columns[colIdx].values) colArr.insert(v);
                    colArr.m_queryColOwner = query_data_retain(arrVar->m_query);
                    colArr.m_queryColIndex = colIdx;
                    colArr.m_queryColFromBracket = true;
                    colArr.m_queryColWritable = true;
                    return colArr;
                }
                string upper = key;
                upper.toUpper();
                if (upper.equals("COLUMNLIST")) return cfvariant(queryColumnList(arrVar));
                if (upper.equals("RECORDCOUNT")) return cfvariant(queryRecordCount(arrVar));
                if (upper.equals("CURRENTROW")) return cfvariant(arrVar->m_query->currentRow);
                throw webstrada::exception("Element '" + key + "' is undefined in Q.");
            }
            string key = idxVal.toString();
            key.toUpper();
            auto it = arrVar->m_struct->find(key);
            if (it != arrVar->m_struct->end()) {
                return it->second;
            }
            throw webstrada::exception("Element '" + key + "' is undefined: " + e);
        }
        if (arrVar && arrVar->m_type != cfvariant::Array && arrVar->m_type != cfvariant::Struct &&
            arrVar->m_type != cfvariant::Xml && arrVar->m_type != cfvariant::Query &&
            arrVar->m_type != cfvariant::String) {
            // Indexing a scalar (e.g. y[1] where y holds the first-cell copy of
            // an integer column) throws CF's dereference error.
            throw webstrada::exception("You have attempted to dereference a scalar variable of type class " +
                scalarJavaTypeName(arrVar) + " as a structure with members.");
        }
    }

    // 6. Check if it is a literal string (handles escaped quotes "" / '',
    //    escaped hashes ## and #expr# interpolation)
    if (e.first() == '"' && e.at(e.length() - 1) == '"') {
        return evaluateStringLiteral(out, e, '"', cgi, server, cookie, application, session, url, form, variables);
    }
    if (e.first() == '\'' && e.at(e.length() - 1) == '\'') {
        return evaluateStringLiteral(out, e, '\'', cgi, server, cookie, application, session, url, form, variables);
    }

    // 7. Check if it is boolean
    if (e.compareCaseInsensitive("true") == 0 || e.compareCaseInsensitive("yes") == 0) {
        cfvariant res(cfvariant::Boolean);
        res.m_bool = true;
        res.m_boolLiteral = true;
        return res;
    }
    if (e.compareCaseInsensitive("false") == 0 || e.compareCaseInsensitive("no") == 0) {
        cfvariant res(cfvariant::Boolean);
        res.m_bool = false;
        res.m_boolLiteral = true;
        return res;
    }

    // 8. Check if it is numeric. Only accept CFML's numeric grammar (digits,
    //    optional '.', optional exponent, optional sign); strtod/strtoll alone
    //    would also accept C99 forms like inf/infinity/nan, which must resolve
    //    as variables instead.
    if (isCfmNumericLiteral(e)) {
        char *end = nullptr;
        errno = 0;
        long long n = strtoll(e.constData(), &end, 10);
        if (*end == '\0' && errno != ERANGE) {
            if (n >= -2147483648LL && n <= 2147483647LL) {
                return cfvariant(static_cast<int>(n));
            }
            // Beyond int32 but within int64 (excluding a leading '-' token, which
            // CF renders as a computed double): promote to Long (2147483648 ->
            // "2147483648", 9223372036854775807 -> "9223372036854775807").
            if (e.at(0) != '-') {
                cfvariant res(cfvariant::Long);
                res.m_long = n;
                return res;
            }
            // '-'-prefixed: computed float (matches CF: -2147483649 -> digits,
            // -9223372036854775808 -> -9.22337203685E+018).
            cfvariant res(cfvariant::Float);
            res.m_double = static_cast<double>(n);
            return res;
        }
        double d = strtod(e.constData(), &end);
        if (*end == '\0') {
            cfvariant res(cfvariant::Float);
            res.m_double = d;
            // Preserve the literal text of unsigned float literals (8.0 -> "8.0",
            // 8.10 -> "8.10", 5.0E2 -> "5.0E2"). A leading '-' makes the value
            // computed (-8.0 -> "-8", matching CF), a leading '+' is stripped
            // (+8.0 -> "8.0").
            if (!e.isEmpty() && e.at(0) != '-') {
                string lit = e;
                if (lit.at(0) == '+') lit = lit.mid(1, lit.length() - 1);
                res.m_literalText = new string(lit);
            }
            return res;
        }
    }

    // 9. Otherwise, treat it as a variable lookup. A variable (or a member of
    //    it) that cannot be resolved must throw, matching ColdFusion, instead
    //    of rendering the raw expression text (previously BUGS.md #4).
    cfvariant *v = lookupVarWritable(e.constData(), cgi, server, cookie, application, session, url, form, variables);
    if (v) {
        return *v;
    }

    // A bare built-in function name (e.g. #pi#, #abs#) evaluates to a method
    // handle rather than throwing; a variable with that name shadows it (the
    // lookup above already won). Verified against CF 2021 (previously
    // BUGS.md #3).
    if (isBareIdentifier(e) && isKnownFunctionName(e)) {
        return makeFunctionHandle(e);
    }

    string root = extractRootVarName(e);
    cfvariant *base = lookupVarWritable(root.constData(), cgi, server, cookie, application, session, url, form, variables);
    if (base) {
        // The root variable exists but a member lookup failed (e.g. st.missingKey).
        string member = extractLastMember(e);
        member.toUpper();
        throw webstrada::exception(string("Element '") + member + "' is undefined");
    }
    // A scope-qualified missing member (session.x, application.y, variables.z,
    // ...) reports CF's "Element X is undefined in SESSION." wording.
    if (cf_throw_scope_member_error(e.constData(),
            static_cast<const webstrada::cfvariant*>(cgi),
            static_cast<const webstrada::cfvariant*>(server),
            static_cast<const webstrada::cfvariant*>(cookie),
            static_cast<const webstrada::cfvariant*>(application),
            static_cast<const webstrada::cfvariant*>(session),
            static_cast<const webstrada::cfvariant*>(url),
            static_cast<const webstrada::cfvariant*>(form),
            static_cast<const webstrada::cfvariant*>(variables))) {
        // unreachable: cf_throw_scope_member_error throws
    }
    // A dotted member on an UNDEFINED base reports CF's ELEMENT message
    // ("Element KEY is undefined in UNDEFINEDSTRUCT.") for a SINGLE-dot access;
    // a multi-dot access (undefinedStruct.a.b) keeps the variable message. Was
    // BUGS.md "chain-base lookups". Scope-qualified names were handled above.
    if (!isBareIdentifier(e)) {
        int dot = e.indexOf('.');
        if (dot > 0) {
            string member = e.mid(dot + 1, e.length() - dot - 1).trimmed();
            if (member.indexOf('.') < 0 && member.indexOf('[') < 0) {
                member.toUpper();
                root.toUpper();
                webstrada::string msg("Element ");
                msg.append(member);
                msg.append(" is undefined in ");
                msg.append(root);
                msg.append(".");
                throw webstrada::exception(msg);
            }
        }
    }
    root.toUpper();
    throw webstrada::exception(string("Variable ") + root + " is undefined.");
}

void cfml::cfoutputexpr(webstrada::string &out, void *cgi, void *server, void *cookie,
                         void *application, void *session, void *url,
                         void *form, void *variables, const char *varName)
{
    if (cfml::response().binary) return; // cfcontent file/variable: other output ignored
    webstrada::string expr(varName);
    cfvariant val = evaluateExpr(out, expr, cgi, server, cookie, application, session, url, form, variables);
    appendExprValueAsString(out, val, varName);
}

const webstrada::cfvariant *cfml::cfgetvar(const webstrada::cfvariant *scope, const char *key)
{
    if (scope->m_type != webstrada::cfvariant::Struct || scope->m_disabled) {
        webstrada::string msg("Variable '");
        msg.append(key);
        msg.append("' not found in scope for cfdump");
        throw webstrada::exception(msg);
    }

    webstrada::string k(key);
    k.toUpper();

    auto it = scope->m_struct->find(k);
    if (it == scope->m_struct->end()) {
        webstrada::string msg("Variable '");
        msg.append(key);
        msg.append("' not found in scope for cfdump");
        throw webstrada::exception(msg);
    }

    return &it->second;
}

// Converts a string to a CFML boolean exactly like CF's Cast._boolean: the
// words yes/true (case-insensitive) and any non-zero number are true, the
// words no/false and zero are false, and anything else (including the empty
// string) throws, matching ColdFusion's BooleanStringConversionException.
static int cfmlStringTruthy(const webstrada::string &s)
{
    if (s.isEmpty()) {
        throw webstrada::exception("The value cannot be converted to a boolean.");
    }
    char c = s.at(0);
    if (c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9')) {
        char *end = nullptr;
        const char *data = s.constData();
        double d = strtod(data, &end);
        if (end && end != data && *end == '\0') {
            return d != 0.0 ? 1 : 0;
        }
        throw webstrada::exception("The value cannot be converted to a boolean.");
    }
    webstrada::string lower = s;
    lower.toLower();
    if (c == 'f' || c == 'F') {
        if (lower.equals("false")) return 0;
    } else if (c == 'n' || c == 'N') {
        if (lower.equals("no")) return 0;
    } else if (c == 't' || c == 'T') {
        if (lower.equals("true")) return 1;
    } else if (c == 'y' || c == 'Y') {
        if (lower.equals("yes")) return 1;
    }
    throw webstrada::exception("The value cannot be converted to a boolean.");
}

int cfml::isTruthy(const webstrada::cfvariant &val)
{
    switch (val.m_type) {
    case webstrada::cfvariant::Boolean: return val.m_bool ? 1 : 0;
    case webstrada::cfvariant::Number:  return val.m_int != 0 ? 1 : 0;
    case webstrada::cfvariant::Long:    return val.m_long != 0 ? 1 : 0;
    case webstrada::cfvariant::Float:   return val.m_double != 0.0 ? 1 : 0;
    case webstrada::cfvariant::String:  return val.m_str ? cfmlStringTruthy(*val.m_str) : 0;
    case webstrada::cfvariant::Array:
        // A query-column reference is truthy by its current cell's value.
        if (val.m_queryColOwner && val.m_queryColIndex >= 0) {
            cfvariant cell = scalarizeQueryColumn(&val);
            return isTruthy(cell);
        }
        return 0;
    default: return 0;
    }
}

static const webstrada::cfvariant *lookupVar(const char *name,
    const webstrada::cfvariant *cgi, const webstrada::cfvariant *server,
    const webstrada::cfvariant *cookie, const webstrada::cfvariant *application,
    const webstrada::cfvariant *session, const webstrada::cfvariant *url,
    const webstrada::cfvariant *form, const webstrada::cfvariant *variables)
{
    // An unqualified name is only searched in the variables scope (plus, inside
    // a UDF, the enclosing function's parent scopes) and — when
    // searchimplicitscopes is enabled — the implicit scopes. SERVER /
    // APPLICATION / SESSION are NEVER searched for unqualified names (matches
    // ColdFusion's searchScopes order: variables scope, then the implicit
    // scopes CGI/FILE/URL/FORM/COOKIE/CLIENT).
    webstrada::string key(name);
    key.toUpper();

    auto findIn = [&](const webstrada::cfvariant *scope) -> const webstrada::cfvariant* {
        if (!scope || scope->m_type != webstrada::cfvariant::Struct || scope->m_disabled)
            return nullptr;
        auto it = scope->m_struct->find(key);
        return (it != scope->m_struct->end()) ? &it->second : nullptr;
    };

    // Inside a UDF / component method the function-local scope is searched
    // first, then the (component) variables scope, then the enclosing UDF
    // parent scopes, then the component's this scope, then the implicit scopes.
    if (!g_udfCtx.empty()) {
        auto localName = key;
        auto loopIt = g_udfCtx.back().loopIndices.find(localName);
        if (loopIt != g_udfCtx.back().loopIndices.end()) {
            return &loopIt->second;
        }
        auto *local = g_udfCtx.back().localScope;
        if (local && local->m_type == cfvariant::Struct && local->m_structData) {
            auto &localMap = local->m_structData->map;
            auto it = localMap.find(key);
            if (it != localMap.end()) return &it->second;
        }
        // A missing parameter is a Null slot in `arguments` and reads as
        // undefined (CF: "Variable A is undefined.").
        if (cfvariant *args = udfArgumentsScope(local)) {
            if (auto *r = findIn(args)) {
                if (r->m_type != cfvariant::Null) return r;
            }
        }
    }
    // A nested component/UDF keeps its local and arguments scopes ahead of
    // the surrounding custom tag's private variables.
    if (g_customTagExecutionVariables) {
        if (auto *r = findIn(g_customTagExecutionVariables)) return r;
    }
    // Adobe CF places implicit query columns after the active function local
    // and arguments scopes, but before the enclosing variables scope.
    std::vector<webstrada::string> queryParts;
    queryParts.push_back(key);
    if (auto *r = query_scope_resolve_member(queryParts)) return r;
    if (auto *r = findIn(variables)) return r;
    for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
        if (auto *r = findIn(it->parentScope)) return r;
        // A closure / nested function captures the enclosing function's local
        // scope; its parameters live in that scope's `arguments` struct.
        if (auto *r = findIn(udfArgumentsScope(it->parentScope))) {
            if (r->m_type != cfvariant::Null) return r;
        }
    }
    for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
        if (it->thisScope) {
            if (auto *r = findIn(it->thisScope)) return r;
            break;
        }
    }
    if (g_searchImplicitScopes) {
        // CF's implicit-scope search order: CGI, FILE, URL, FORM, COOKIE, CLIENT.
        // FILE / CLIENT are not implemented; the rest are searched in order.
        const webstrada::cfvariant *implicitScopes[] = {cgi, url, form, cookie};
        for (auto *s : implicitScopes) {
            if (auto *r = findIn(s)) return r;
        }
    }
    return nullptr;
}

// CF's Utils.isBooleanOrNumericOrDateCandidate: the gate applied when BOTH
// operands of a comparison are strings — only then may they be cast to a
// number for the numeric compare (CfJspPage._compare). Non-candidates always
// compare lexicographically (case-insensitive).
static bool isBooleanOrNumericOrDateCandidate(const string &s)
{
    string t = s;
    t = t.trimmed();
    if (t.isEmpty()) return false;
    if (t.at(t.length() - 1) == '.') return false;
    switch (t.at(0)) {
    case '+': case '-': case '.':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case 'D': case 'F': case 'J': case 'M': case 'N': case 'O':
    case 'S': case 'T': case 'Y':
    case 'a': case 'd': case 'f': case 'j': case 'm': case 'n':
    case 'o': case 's': case 't': case 'y':
    case '{':
        return true;
    default:
        return false;
    }
}

// CF's Cast._double(v, true, false): converts a Number/Long/Float/Boolean/
// DateTime to a double and a String by first-char dispatch — numeric strings
// and the boolean tokens true/yes/false/no are numbers, everything else falls
// back to a date-serial parse; anything unconvertible reports false (null).
static bool tryCfDouble(const cfvariant *v, double &out)
{
    if (!v) return false;
    switch (v->m_type) {
    case cfvariant::Number:   out = static_cast<double>(v->m_int); return true;
    case cfvariant::Long:     out = static_cast<double>(v->m_long); return true;
    case cfvariant::Float:    out = v->m_double; return true;
    case cfvariant::Boolean:  out = v->m_bool ? 1.0 : 0.0; return true;
    case cfvariant::DateTime: out = v->m_double; return true;
    case cfvariant::String: {
        string t = *v->m_str;
        t = t.trimmed();
        if (t.isEmpty()) return false;
        char c = t.at(0);
        // CF's first-char dispatch (Cast._double): '+','-','.' strings are
        // numeric-only (no date fallback); digit/date-letter/f/n strings fall
        // through to a date-serial parse when they are not a plain number or
        // boolean token; 't'/'y' (not true/yes) and other letters are null.
        if (c == '+' || c == '-' || c == '.') {
            if (isCfmNumericLiteral(t)) {
                char *end = nullptr;
                double d = strtod(t.constData(), &end);
                if (end && *end == '\0') { out = d; return true; }
            }
            return false;
        }
        if (c >= '0' && c <= '9') {
            if (isCfmNumericLiteral(t)) {
                char *end = nullptr;
                double d = strtod(t.constData(), &end);
                if (end && *end == '\0') { out = d; return true; }
            }
        } else if (c == 'f' || c == 'F') {
            if (t.compareCaseInsensitive("false") == 0) { out = 0.0; return true; }
        } else if (c == 'n' || c == 'N') {
            if (t.compareCaseInsensitive("no") == 0) { out = 0.0; return true; }
        } else if (c == 't' || c == 'T') {
            if (t.compareCaseInsensitive("true") == 0) { out = 1.0; return true; }
            return false;
        } else if (c == 'y' || c == 'Y') {
            if (t.compareCaseInsensitive("yes") == 0) { out = 1.0; return true; }
            return false;
        }
        double d = 0.0;
        if (parseDateTimeStr(t, d)) { out = d; return true; }
        return false;
    }
    default:
        return false;
    }
}

// CF's CfJspPage._compare(a, b) — the primitive behind EQ/NEQ/GT/GTE/LT/LTE.
// Returns negative/zero/positive. When both operands are strings they only
// participate in the numeric cast if both are boolean/numeric/date candidates;
// a failed cast falls back to a case-insensitive string comparison. Complex
// values (arrays/structs/queries/binary) never compare equal (CF throws
// ComplexObjectException; the engine reports "not equal", matching the
// pre-existing behavior).
static int cfCompare(const cfvariant *a, const cfvariant *b)
{
    auto isComplex = [](const cfvariant *v) {
        return v->m_type == cfvariant::Array || v->m_type == cfvariant::Struct ||
               v->m_type == cfvariant::Query || v->m_type == cfvariant::Binary ||
               v->m_type == cfvariant::Function;
    };
    if (isComplex(a) || isComplex(b)) return 1;

    bool bothString = a->m_type == cfvariant::String && b->m_type == cfvariant::String;
    bool castToDouble = true;
    if (bothString) {
        castToDouble = isBooleanOrNumericOrDateCandidate(*a->m_str) &&
                       isBooleanOrNumericOrDateCandidate(*b->m_str);
    }
    if (castToDouble) {
        double da = 0.0, db = 0.0;
        if (tryCfDouble(a, da) && tryCfDouble(b, db)) {
            return (da == db) ? 0 : (da < db ? -1 : 1);
        }
    }
    // Strict fallback: Cast._String both and compare case-insensitively.
    // toString() is non-const, so stringify non-const copies (cheap scalars).
    cfvariant ac = *a;
    cfvariant bc = *b;
    string sa = ac.toString();
    string sb = bc.toString();
    int r = sa.compareCaseInsensitive(sb);
    return r < 0 ? -1 : (r > 0 ? 1 : 0);
}

int compareVariants(const webstrada::cfvariant *a, const webstrada::cfvariant *b, const webstrada::string &op)
{
    if (!a || !b) return 0;

    // A query-column reference compares as its current row's scalar cell
    // (`q.id EQ 2` inside a <cfloop query>), like any other scalar.
    cfvariant as = scalarizeQueryColumn(a);
    cfvariant bs = scalarizeQueryColumn(b);
    a = &as;
    b = &bs;

    int cmp = cfCompare(a, b);
    if (op.equals("EQ") || op.equals("IS") || op.equals("EQUAL") || op.equals("==")) return cmp == 0 ? 1 : 0;
    if (op.equals("NEQ") || op.equals("IS NOT") || op.equals("NOT EQUAL") || op.equals("!=")) return cmp != 0 ? 1 : 0;
    if (op.equals("GT") || op.equals("GREATER THAN") || op.equals(">")) return cmp > 0 ? 1 : 0;
    if (op.equals("GTE") || op.equals("GE") || op.equals("GREATER THAN OR EQUAL TO") || op.equals(">=")) return cmp >= 0 ? 1 : 0;
    if (op.equals("LT") || op.equals("LESS THAN") || op.equals("<")) return cmp < 0 ? 1 : 0;
    if (op.equals("LTE") || op.equals("LE") || op.equals("LESS THAN OR EQUAL TO") || op.equals("<=")) return cmp <= 0 ? 1 : 0;
    if (op.equals("CONTAINS")) {
        if (a->m_type == webstrada::cfvariant::String && b->m_type == webstrada::cfvariant::String)
            return a->m_str->containsCaseInsensitive(b->m_str->constData()) ? 1 : 0;
        return 0;
    }
    if (op.equals("DOES NOT CONTAIN")) return compareVariants(a, b, "CONTAINS") ? 0 : 1;
    if (op.equals("EQV")) return isTruthy(*a) == isTruthy(*b) ? 1 : 0;
    if (op.equals("IMP")) return (!isTruthy(*a) || isTruthy(*b)) ? 1 : 0;
    return 0;
}

int cfml::cfevalbool(const char *expr,
                      const webstrada::cfvariant *cgi, const webstrada::cfvariant *server,
                      const webstrada::cfvariant *cookie, const webstrada::cfvariant *application,
                      const webstrada::cfvariant *session, const webstrada::cfvariant *url,
                      const webstrada::cfvariant *form, const webstrada::cfvariant *variables)
{
    webstrada::string e(expr);
    e = e.trimmed();
    if (e.isEmpty()) return 0;

    webstrada::string eUpper = e;
    eUpper.toUpper();

    static const char *ops[] = {
        " DOES NOT CONTAIN ", " GREATER THAN OR EQUAL TO ", " LESS THAN OR EQUAL TO ",
        " GREATER THAN ", " LESS THAN ",
        " EQ ", " NEQ ", " GT ", " GTE ", " LT ", " LTE ",
        " GE ", " LE ", " IS ", " CONTAINS ", " EQUAL ",
        nullptr
    };

    int opPos = -1;
    int opLen = 0;
    for (int i = 0; ops[i]; i++) {
        int pos = eUpper.indexOf(ops[i]);
        if (pos >= 0) { opPos = pos; opLen = webstrada::string(ops[i]).length(); break; }
    }

    if (opPos < 0) {
        // Check for literal boolean/yes/no/null
        if (e.equals("true") || e.equals("yes")) return 1;
        if (e.equals("false") || e.equals("no")) return 0;
        if (e.equals("null")) return 0;
        // Check for numeric literal. Only CFML's numeric grammar (digits,
        // optional '.', optional exponent, optional sign) is accepted; C99
        // forms like inf/infinity/nan must fall through to a variable lookup
        // (e.g. <cfset inf = 2^1024>), not be treated as literals.
        if (isCfmNumericLiteral(e)) {
            char *ep1 = nullptr;
            long nv = strtol(e.constData(), &ep1, 10);
            if (*ep1 == '\0') return nv != 0;
            double dv = strtod(e.constData(), &ep1);
            if (*ep1 == '\0') return dv != 0.0;
        }
        // Check for string literal
        if ((e.first() == '"' && e.at(e.length()-1) == '"') ||
            (e.first() == '\'' && e.at(e.length()-1) == '\''))
            return 1;
        auto *var = lookupVar(e.constData(), cgi, server, cookie, application, session, url, form, variables);
        return var ? isTruthy(*var) : 0;
    }

    webstrada::string left, right, opStr;
    for (int i = 0; ops[i]; i++) {
        int pos = eUpper.indexOf(ops[i]);
        if (pos == opPos) {
            opStr = webstrada::string(ops[i]).trimmed();
            left = e.left(pos).trimmed();
            right = e.mid(pos + webstrada::string(ops[i]).length(), e.length() - pos - webstrada::string(ops[i]).length()).trimmed();
            break;
        }
    }

    if (left.isEmpty() || right.isEmpty()) return 0;

    std::vector<webstrada::cfvariant> temps;
    // Reserve space to prevent invalidation of pointers after emplace_back
    temps.reserve(16);

    std::function<const webstrada::cfvariant*(const webstrada::string &)> resolve =
        [&](const webstrada::string &s) -> const webstrada::cfvariant* {
        if ((s.first() == '"' && s.at(s.length()-1) == '"') ||
            (s.first() == '\'' && s.at(s.length()-1) == '\'')) {
            webstrada::string sv = s.mid(1, s.length()-2);
            temps.emplace_back(sv);
            return &temps.back();
        }
        if (s.equals("true") || s.equals("yes")) {
            temps.emplace_back(webstrada::cfvariant::Boolean);
            temps.back().m_bool = true;
            return &temps.back();
        }
        if (s.equals("false") || s.equals("no")) {
            temps.emplace_back(webstrada::cfvariant::Boolean);
            temps.back().m_bool = false;
            return &temps.back();
        }
        if (s.equals("null")) return nullptr;
        // Only CFML's numeric grammar is a literal; C99 forms like
        // inf/infinity/nan must resolve as variables.
        if (isCfmNumericLiteral(s)) {
            char *end = nullptr;
            errno = 0;
            long long n = strtoll(s.constData(), &end, 10);
            if (*end == '\0' && errno != ERANGE) {
                if (n >= -2147483648LL && n <= 2147483647LL) {
                    temps.emplace_back(static_cast<int>(n));
                } else if (s.at(0) != '-') {
                    // Beyond int32 within int64: Long (matches CF's Long literals).
                    temps.emplace_back(webstrada::cfvariant::Long);
                    temps.back().m_long = n;
                } else {
                    // Beyond int32: promote to float so large literals compare
                    // numerically (2147483648 GT 2147483647 -> true).
                    temps.emplace_back(webstrada::cfvariant::Float);
                    temps.back().m_double = static_cast<double>(n);
                }
                return &temps.back();
            }
            double d = strtod(s.constData(), &end);
            if (*end == '\0') {
                temps.emplace_back(webstrada::cfvariant::Float);
                temps.back().m_double = d;
                return &temps.back();
            }
        }
        // Exponentiation operator '^' (left-associative in CF: 2^3^2 == 64).
        // Split at the rightmost top-level '^' and resolve both sides.
        {
            int caret = -1;
            for (int i = 0; i < s.length(); i++) {
                if (s.at(i) == '^') caret = i;
            }
            if (caret > 0 && caret < s.length() - 1) {
                webstrada::string l = s.left(caret).trimmed();
                webstrada::string r = s.mid(caret + 1, s.length() - caret - 1).trimmed();
                if (!l.isEmpty() && !r.isEmpty()) {
                    // Snapshot each side immediately: recursive resolve() may
                    // reallocate the temps vector and invalidate earlier pointers.
                    const webstrada::cfvariant *la = resolve(l);
                    if (!la) return nullptr;
                    double da = getDoubleValue(*la);
                    const webstrada::cfvariant *ra = resolve(r);
                    if (!ra) return nullptr;
                    double db = getDoubleValue(*ra);
                    temps.emplace_back(webstrada::cfvariant::Float);
                    temps.back().m_double = std::pow(da, db);
                    return &temps.back();
                }
            }
        }
        return lookupVar(s.constData(), cgi, server, cookie, application, session, url, form, variables);
    };

    auto *a = resolve(left);
    auto *b = resolve(right);
    return compareVariants(a, b, opStr);
}

void cfml::cfset(webstrada::cfvariant *scope, const char *key, const char *value)
{
    webstrada::string k(key);
    k.toUpper();

    webstrada::string v(value);
    v = v.trimmed();

    webstrada::string dummyOut;
    cfvariant val = evaluateExpr(dummyOut, v, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, scope);

    webstrada::cfvariant &slot = scope->set(k);
    slot = std::move(val);
    storeQueryColumnRef(slot);
}

int cfml::cfloop_resolve_int(const char *expr,
    const webstrada::cfvariant *cgi, const webstrada::cfvariant *server,
    const webstrada::cfvariant *cookie, const webstrada::cfvariant *application,
    const webstrada::cfvariant *session, const webstrada::cfvariant *url,
    const webstrada::cfvariant *form, const webstrada::cfvariant *variables)
{
    webstrada::string e(expr);
    e = e.trimmed();
    if (e.length() >= 2 && e.first() == '#' && e.at(e.length()-1) == '#')
        e = e.mid(1, e.length() - 2).trimmed();
    char *end = nullptr;
    long n = strtol(e.constData(), &end, 10);
    if (*end == '\0') return static_cast<int>(n);
    auto *v = lookupVar(e.constData(), cgi, server, cookie, application, session, url, form, variables);
    if (!v) return 0;
    switch (v->m_type) {
        case webstrada::cfvariant::Number: return v->m_int;
        case webstrada::cfvariant::Long: return static_cast<int>(v->m_long);
        case webstrada::cfvariant::Float: return static_cast<int>(v->m_double);
        case webstrada::cfvariant::String: return atoi(v->m_str->constData());
        case webstrada::cfvariant::Boolean: return v->m_bool ? 1 : 0;
        default: return 0;
    }
}

void cfml::cfloop_set_int(webstrada::cfvariant *scope, const char *key, int val)
{
    webstrada::string k(key);
    k.toUpper();
    if (!g_udfCtx.empty()) {
        g_udfCtx.back().loopIndices[k] = webstrada::cfvariant(val);
    }
    webstrada::cfvariant &slot = (scope->m_structData &&
                                  (scope->m_type == webstrada::cfvariant::Struct ||
                                   scope->m_type == webstrada::cfvariant::Component))
        ? scope->m_structData->map[k]
        : scope->set(k);
    slot.set_type(webstrada::cfvariant::Number);
    slot.m_int = val;
}

void cfml::cfloop_set_long(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, cfvariant *variables,
    const char *key, long long val)
{
    // The loop index is assigned like an unqualified <cfset>: a var-declared
    // loop variable lives in the function's local scope (a component method's
    // `variables` is the instance scope, NOT the local scope — writing the
    // index there leaves the local `var node` untouched and the loop body
    // reads an empty value, a WebStrada divergence from CF).
    // A numeric loop index is a function-local binding while a UDF is active.
    // This also covers component methods whose tag-form `var` declarations are
    // not present in the generated local-name table (the index must not fall
    // through to an implicit query column or the component variables scope).
    webstrada::cfvariant *scope = g_udfCtx.empty()
        ? variables : g_udfCtx.back().localScope;
    if (!scope || (scope->m_type != webstrada::cfvariant::Struct && scope->m_type != webstrada::cfvariant::Component)) {
        throw webstrada::exception("Cannot assign variable: target scope is not a valid structure.");
    }
    webstrada::string k(key);
    k.toUpper();
    if (!g_udfCtx.empty()) {
        g_udfCtx.back().loopIndices[k] = webstrada::cfvariant(static_cast<int>(val));
    }
    webstrada::cfvariant &slot = scope->set(k);
    if (val >= INT32_MIN && val <= INT32_MAX) {
        slot.set_type(webstrada::cfvariant::Number);
        slot.m_int = static_cast<int>(val);
    } else {
        // Beyond int32: store as a computed double so `#i#` renders like CF
        // (2147483648 -> "2147483648", 1000000000000 -> "1E+012"). CF runs the
        // counter in floating point, so a Long counter here would diverge
        // (verified against CF 2021 in tests/cfm/cfloop_bounds_test.cfm).
        slot.set_type(webstrada::cfvariant::Float);
        slot.m_double = static_cast<double>(val);
    }
}

void cfml::cfloop_assign_index(
    const cfvariant *cgi, const cfvariant *server,
    const cfvariant *cookie, const cfvariant *application,
    const cfvariant *session, const cfvariant *url,
    const cfvariant *form, cfvariant *variables,
    const char *name, const cfvariant *value)
{
    webstrada::cfvariant *scope = udfAssignScope(variables, name);
    if (!scope || (scope->m_type != webstrada::cfvariant::Struct && scope->m_type != webstrada::cfvariant::Component)) {
        throw webstrada::exception("Cannot assign variable: target scope is not a valid structure.");
    }
    webstrada::cfvariant &slot = scope->set(name);
    slot = *value;
    storeQueryColumnRef(slot);
}

void cfml::cfabort(void)
{
    throw webstrada::abort_exception();
}

// <cfexit> / script `exit;` outside a function body: abort the currently
// executing template page (exit_exception, uncatchable by CFML catch blocks).
// Inside a custom tag the tag runtime catches it; `method` distinguishes
// exittag (kind 0) from exittemplate (kind 1) so the custom tag start template
// can decide whether the body still runs.
void cfml::cf_exit(const cfvariant *method)
{
    int kind = 0;
    if (method && method->m_type != cfvariant::Null) {
        webstrada::string low = const_cast<cfvariant*>(method)->toString();
        low.toLower();
        if (low.equals("exittemplate")) kind = 1;
    }
    throw webstrada::exit_exception(kind);
}

// <cfexit method="loop">: only valid inside a custom tag in end mode.
void cfml::cf_exit_loop(void)
{
    if (!g_customTagStack.empty()) {
        CustomTagCallCtx &ctx = g_customTagStack.back();
        // Check if in end mode
        std::string mode = ctx.thisTag.has("executionMode") ? ctx.thisTag["executionMode"].toString().constData() : "";
        for (auto &c : mode) c = tolower(c);
        if (mode == "end") {
            ctx.loopRequested = true;
            throw webstrada::exit_exception();
        } else {
            throw webstrada::exception("jakarta.servlet.jsp.JspException",
                                      "Exit method Loop not allowed from start tag", "");
        }
    }
    throw webstrada::exception("Application", "Invalid use of the cfexit tag.",
                              "cfexit method=\"loop\" can be used only inside custom tags. The current template is not a custom tag.");
}

// Runtime method-attribute validation for a dynamically evaluated method value
// (`method="#x#"`): catchable Template exception with CF's message. CF renders
// an empty value as '' (see <cferror> runtime messages).
void cfml::cf_exit_invalid(const cfvariant *method)
{
    webstrada::string m = method ? const_cast<cfvariant*>(method)->toString() : webstrada::string();
    webstrada::string msg = webstrada::string("Attribute validation error for CFEXIT.");
    webstrada::string detail("The value of the METHOD attribute, which is currently ");
    if (m.isEmpty()) {
        detail.append("''");
    } else {
        detail.append(m.constData(), m.length());
    }
    detail.append(", must be one of the values: EXITTAG,LOOP,EXITTEMPLATE.");
    throw webstrada::exception("Template", msg, detail);
}

// Classifies an evaluated `method` attribute value for runtime dispatch:
// 0 = exittag/exittemplate (exit the current page / return from function),
// 1 = loop (invalid outside a custom tag), 2 = anything else (invalid method).
// An explicitly empty value (method="") is invalid too — CF throws "The value
// of the METHOD attribute, which is currently ''..." (verified on the RDS host;
// the value is quoted like <cferror>'s). Only a *missing* method is exit.
int cfml::cf_exit_classify(const cfvariant *method)
{
    if (!method) return 0;
    webstrada::string low = const_cast<cfvariant*>(method)->toString();
    low.toLower();
    if (low.isEmpty() || low.equals("exittag") || low.equals("exittemplate")) {
        if (low.isEmpty()) return 2;
        return 0;
    }
    if (low.equals("loop")) return 1;
    return 2;
}

/**
 * @file fn_isvalid.cpp
 * @brief CFML isvalid() built-in.
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
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace cfml {

namespace {

cfvariant *makeBoolResult(bool b) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = b;
    return ret;
}

// Cast._double for a lenient numeric string (used by numeric_legacy).
bool stringToDoubleLenient(const std::string &s, double &out) {
    if (s.empty()) return false;
    char *endptr = nullptr;
    out = std::strtod(s.c_str(), &endptr);
    while (endptr && *endptr && std::isspace(static_cast<unsigned char>(*endptr))) endptr++;
    return endptr && (*endptr == '\0');
}

bool isValidEmail(const std::string &email) {
    // Minimal InternetAddress-style sanity: local@domain, domain has a dot
    // and a TLD of 2+ letters (already enforced by the regex).
    return true;
}

} // namespace

cfvariant *cf_isvalid(const cfvariant *type, const cfvariant *value,
                      const cfvariant *min, const cfvariant *max,
                      const cfvariant *pattern) {
    if (!type) throw webstrada::exception("IsValid requires at least 2 arguments");
    std::string typeStr = const_cast<cfvariant*>(type)->toString().constData();
    // CF: the type is trimmed and uppercased.
    std::string t = typeStr;
    size_t tb = 0, te = t.size();
    while (tb < te && std::isspace(static_cast<unsigned char>(t[tb]))) tb++;
    while (te > tb && std::isspace(static_cast<unsigned char>(t[te - 1]))) te--;
    t = t.substr(tb, te - tb);
    for (auto &c : t) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    bool result = false;

    // The 3-arg form IsValid(type, value, pattern) passes the pattern in the
    // third positional slot (min). Detect it: min set, max and pattern null.
    if (min && !max && !pattern) {
        pattern = min;
        min = nullptr;
    }

    // 4-arg RANGE form.
    if (min && max) {
        if (t != "RANGE") {
            throw webstrada::exception("InvalidValidationTypeException",
                                      "Valid type arguments are: any, array, Boolean, date, numeric, query, string, struct, UUID, GUID, binary, integer, float, eurodate, time, creditcard, email, ssn, telephone, zipcode, url, regex, range , component, and variableName.",
                                      "");
        }
        if (!value) return makeBoolResult(false);
        std::string vs = const_cast<cfvariant*>(value)->toString().constData();
        double v = 0;
        if (!cfmlStrictParseDouble(vs, v)) return makeBoolResult(false);
        double lo = getDoubleValue(*const_cast<cfvariant*>(min));
        double hi = getDoubleValue(*const_cast<cfvariant*>(max));
        result = (v >= lo && v <= hi);
        return makeBoolResult(result);
    }

    if (t == "REGEX" || t == "REGULAR_EXPRESSION") {
        if (!pattern) {
            throw webstrada::exception("InvalidRegexValidationTypeException",
                                      "Valid type arguments are: any, array, Boolean, date, numeric, query, string, struct, UUID, GUID, binary, integer, float, eurodate, time, creditcard, email, ssn, telephone, zipcode, url, regex, range , component, and variableName.",
                                      "");
        }
        if (!value) return makeBoolResult(false);
        std::string vs = const_cast<cfvariant*>(value)->toString().constData();
        std::string pat = const_cast<cfvariant*>(pattern)->toString().constData();
        // CF validates the trimmed value with REMatch (substring find).
        size_t b = 0, e = vs.size();
        while (b < e && std::isspace(static_cast<unsigned char>(vs[b]))) b++;
        while (e > b && std::isspace(static_cast<unsigned char>(vs[e - 1]))) e--;
        std::string trimmed = vs.substr(b, e - b);
        return makeBoolResult(cfmlRegexFullMatch(pat, trimmed));
    }

    if (t == "RANGE") {
        throw webstrada::exception("InvalidRangeValidationTypeException",
                                  "Valid type arguments are: any, array, Boolean, date, numeric, query, string, struct, UUID, GUID, binary, integer, float, eurodate, time, creditcard, email, ssn, telephone, zipcode, url, regex, range , component, and variableName.",
                                  "");
    }

    if (t == "COMPONENT") {
        return makeBoolResult(value && value->m_type == cfvariant::Component);
    }

    if (t == "URL") {
        if (!value) return makeBoolResult(false);
        std::string thisvalue = const_cast<cfvariant*>(value)->toString().constData();
        size_t b = 0, e = thisvalue.size();
        while (b < e && std::isspace(static_cast<unsigned char>(thisvalue[b]))) b++;
        while (e > b && std::isspace(static_cast<unsigned char>(thisvalue[e - 1]))) e--;
        std::string lv = thisvalue.substr(b, e - b);
        for (auto &c : lv) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lv.find('[') != std::string::npos && lv.find(']') != std::string::npos) {
            return makeBoolResult(cfmlRegexFullMatch(
                "^((http|https|ftp|file)\\:\\/\\/([a-zA-Z0-0]*:[a-zA-Z0-0]*(@))?(\\[)?[a-zA-Z0-9-\\.:]+(\\])?(\\.[a-zA-Z]{2,3})?(:[a-zA-Z0-9]*)?\\/?(([a-zA-Z0-9-\\._\\?\\,\\'\\/\\+&:@\\$#\\=~\\*\\!\\(\\)])|(%[a-f0-9]{2}))*)|((news)\\:[a-zA-Z0-9\\.]*)$",
                lv));
        }
        return makeBoolResult(cfmlIsValidUrl(lv));
    }

    if (t == "ANY") {
        if (!value) return makeBoolResult(false);
        auto *sv = cf_issimplevalue(value);
        cf_register_temp(sv);
        bool r = sv->m_bool;
        return makeBoolResult(r);
    }

    if (t == "ARRAY") {
        return makeBoolResult(value && value->m_type == cfvariant::Array);
    }
    if (t == "BINARY") {
        return makeBoolResult(value && value->m_type == cfvariant::Binary);
    }
    if (t == "BOOLEAN" || t == "BOOL") {
        if (!value) return makeBoolResult(false);
        auto *res = cf_isboolean(value);
        cf_register_temp(res);
        bool r = res->m_bool;
        return makeBoolResult(r);
    }
    if (t == "DATE") {
        if (!value) return makeBoolResult(false);
        if (value->m_type == cfvariant::DateTime) return makeBoolResult(true);
        double dummy = 0;
        if (parseDateTimeStr(const_cast<cfvariant*>(value)->toString(), dummy)) return makeBoolResult(true);
        auto *nd = cf_isnumericdate(value);
        cf_register_temp(nd);
        bool r = nd->m_bool;
        return makeBoolResult(r);
    }
    if (t == "DATETIME_OBJECT" || t == "DATETIMEOBJECT" || t == "DATETIME") {
        return makeBoolResult(value && value->m_type == cfvariant::DateTime);
    }
    if (t == "GUID") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        return makeBoolResult(cfmlRegexFullMatch("[A-Fa-f0-9]{8,8}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{12,12}", s));
    }
    if (t == "UUID") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        return makeBoolResult(cfmlRegexFullMatch("[A-Fa-f0-9]{8,8}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{16,16}", s));
    }
    if (t == "NUMERIC" || t == "NUMBER") {
        return makeBoolResult(cfmlIsValidNumeric(value));
    }
    if (t == "NUMERIC_LEGACY") {
        if (!value) return makeBoolResult(false);
        double v = 0;
        if (value->m_type == cfvariant::Number || value->m_type == cfvariant::Float ||
            value->m_type == cfvariant::Long || value->m_type == cfvariant::Boolean ||
            value->m_type == cfvariant::DateTime) {
            try { v = getDoubleValue(*const_cast<cfvariant*>(value)); result = true; }
            catch (...) { result = false; }
        } else {
            std::string s = const_cast<cfvariant*>(value)->toString().constData();
            result = cfmlStrictParseDouble(s, v) || stringToDoubleLenient(s, v);
        }
        return makeBoolResult(result);
    }
    if (t == "INTEGER") {
        if (!value) return makeBoolResult(false);
        result = cfmlIsValidInteger(value);
        return makeBoolResult(result);
    }
    if (t == "FLOAT") {
        return makeBoolResult(cfmlIsValidNumeric(value));
    }
    if (t == "QUERY") {
        return makeBoolResult(value && value->m_type == cfvariant::Query);
    }
    if (t == "STRING") {
        if (!value) return makeBoolResult(false);
        return makeBoolResult(value->m_type != cfvariant::Array && value->m_type != cfvariant::Struct &&
                              value->m_type != cfvariant::Query && value->m_type != cfvariant::Xml &&
                              value->m_type != cfvariant::Binary && value->m_type != cfvariant::Image &&
                              value->m_type != cfvariant::Component && value->m_type != cfvariant::JSon &&
                              value->m_type != cfvariant::Function);
    }
    if (t == "STRUCT") {
        return makeBoolResult(value && value->m_type == cfvariant::Struct);
    }
    if (t == "VARIABLE" || t == "VARIABLENAME") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        result = cfmlIsValidVariableName(s);
        return makeBoolResult(result);
    }
    if (t == "XML") {
        if (!value) return makeBoolResult(false);
        if (value->m_type == cfvariant::Xml) return makeBoolResult(true);
        try {
            auto *x = cf_isxml(value);
            cf_register_temp(x);
            return makeBoolResult(x->m_bool);
        } catch (...) {
            return makeBoolResult(false);
        }
    }
    if (t == "FUNCTION") {
        return makeBoolResult(value && (value->m_type == cfvariant::Function || value->m_udf != nullptr));
    }
    if (t == "USDATE") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        return makeBoolResult(cfmlIsUsDate(s));
    }
    if (t == "EURODATE") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        return makeBoolResult(cfmlIsEuroDate(s));
    }
    if (t == "TIME") {
        if (!value) return makeBoolResult(false);
        if (value->m_type == cfvariant::DateTime) return makeBoolResult(true);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        if (cfmlIsTimeString(s)) return makeBoolResult(true);
        double dummy = 0;
        return makeBoolResult(parseDateTimeStr(const_cast<cfvariant*>(value)->toString(), dummy));
    }
    if (t == "CREDITCARD") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        std::string cc;
        for (char c : s) if (std::isdigit(static_cast<unsigned char>(c))) cc += c;
        if (cc.size() < 13 || cc.size() > 19) return makeBoolResult(false);
        double v = 0;
        if (!cfmlStrictParseDouble(cc, v)) return makeBoolResult(false);
        return makeBoolResult(cfmlLuhnCheck(cc));
    }
    if (t == "EMAIL") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        if (s.find(',') != std::string::npos) return makeBoolResult(false);
        if (!cfmlRegexFullMatch("^[a-zA-Z_0-9-'\\+~]+(\\.[a-zA-Z_0-9-'\\+~]+)*@([a-zA-Z_0-9-]+\\.)+[a-zA-Z]{2,7}$", s))
            return makeBoolResult(false);
        return makeBoolResult(isValidEmail(s));
    }
    if (t == "SSN" || t == "SOCIAL_SECURITY_NUMBER") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        return makeBoolResult(cfmlRegexFullMatch("^[0-9]{3}(-| )[0-9]{2}(-| )[0-9]{4}$", s));
    }
    if (t == "TELEPHONE") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        return makeBoolResult(cfmlRegexFullMatch(
            "^(((1))?[ ,\\-,\\.]?([\\(]?([1-9][0-9]{2})[\\)]?))?[ ,\\-,\\.]?([^0-1]){1}([0-9]){2}[ ,\\-,\\.]?([0-9]){4}(( )((x){0,1}([0-9]){1,5}){0,1})?$",
            s));
    }
    if (t == "ZIPCODE") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        return makeBoolResult(cfmlRegexFullMatch("^([0-9]){5,5}$|(([0-9]){5,5}(-| ){1}([0-9]){4,4}$)", s));
    }
    if (t == "URLIPV6") {
        if (!value) return makeBoolResult(false);
        std::string s = const_cast<cfvariant*>(value)->toString().constData();
        return makeBoolResult(cfmlRegexFullMatch(
            "^((http|https|ftp|file)\\:\\/\\/([a-zA-Z0-0]*:[a-zA-Z0-0]*(@))?(\\[)?[a-zA-Z0-9-\\.:]+(\\])?(\\.[a-zA-Z]{2,3})?(:[a-zA-Z0-9]*)?\\/?(([a-zA-Z0-9-\\._\\?\\,\\'\\/\\+&:@\\$#\\=~\\*\\!\\(\\)])|(%[a-f0-9]{2}))*)|((news)\\:[a-zA-Z0-9\\.]*)$",
            s));
    }
    if (t == "TYPED_ARRAY") {
        return makeBoolResult(value && value->m_type == cfvariant::Array);
    }

    // Unknown type: CF throws IllegalParamTypeException.
    throw webstrada::exception("IllegalParamTypeException",
                              "Valid type arguments are: any, array, Boolean, date, numeric, query, string, struct, UUID, GUID, binary, integer, float, eurodate, time, creditcard, email, ssn, telephone, zipcode, url, regex, range , component, and variableName.",
                              "");
}

} // namespace cfml

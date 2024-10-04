/**
 * @file tag_param.cpp
 * @brief <cfparam> and <cfobjectcache> runtime implementations.
 *
 * <cfparam> mirrors ColdFusion's ParamTag (coldfusion/tagext/lang/ParamTag):
 * tests whether a named parameter exists (searching the variables scope plus
 * the implicit CGI/URL/FORM/COOKIE scopes for unqualified names, like CF's
 * pageContext.findAttribute), optionally assigns a default, then validates the
 * value against the requested CF type. Error messages / types / detail text are
 * reproduced byte-for-byte from CF 2025 (verified on the RDS host).
 *
 * <cfobjectcache action="clear"> flushes the query cache (CF's
 * ObjectCacheTag -> DataSourceService.purgeQueryCache), which here is the
 * CacheStore's QUERY region.
 */

#include "common.h"
#include "../cffunctions/common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/cache_store.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace cfml {

namespace {

// ---- CF's ParamTag exception constructors ----

// Internal carrier for a failed type validation (CF's CFTypeValidationException
// subclasses). The dispatch wraps it in CF's InvalidParamTypeException
// ("Invalid parameter type." + the validator's message+detail concatenated).
struct paramTypeError {
    std::string msg;
    std::string detail;
};

// ParameterNotFoundException: type Expression.
[[noreturn]] void throwParamNotFound(const std::string &name, bool upperName)
{
    std::string shown = name;
    if (upperName) {
        for (char &c : shown) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    throw webstrada::exception(
        webstrada::string("The required parameter ") + webstrada::string(shown.c_str()) +
            webstrada::string(" was not provided."),
        webstrada::string("This page uses the cfparam tag to declare the parameter ") +
            webstrada::string(shown.c_str()) +
            webstrada::string(" as required for this template. The parameter is not available. "
                             "Ensure that you have passed or initialized the parameter correctly. "
                             "To set a default value for the parameter, use the default attribute of the cfparam tag."));
}

// InvalidParamTypeException: message "Invalid parameter type.", detail is the
// validator's message+detail concatenated (CF's ex.getMessage()+ex.getDetail()).
[[noreturn]] void throwInvalidParamType(const std::string &validatorMsg, const std::string &validatorDetail)
{
    throw webstrada::exception(webstrada::string("Invalid parameter type."),
                              webstrada::string(validatorMsg.c_str()) + webstrada::string(validatorDetail.c_str()));
}

// IllegalParamTypeException: an unknown/unsupported type attribute value.
[[noreturn]] void throwIllegalParamType()
{
    throw webstrada::exception(webstrada::string("The type attribute has an invalid value."),
        webstrada::string("The value of the attribute must be any, array, Boolean, date, numeric, query, string, "
                         "struct, UUID, GUID, binary, integer, float, eurodate, time, creditcard, email, ssn, "
                         "telephone, zipcode, url, regex, range, social_security_number, USdate, XML,  or variableName."));
}

// InvalidSimpleTypeException(type) — carried as a paramTypeError.
[[noreturn]] void throwInvalidSimpleType(const char *type)
{
    throw paramTypeError{
        "The value cannot be converted to a " + std::string(type) + " because it is  not a simple value.",
        "Simple values are booleans, numbers,  strings, and date-time values."
    };
}

// InvalidComplexTypeException(type) — carried as a paramTypeError.
[[noreturn]] void throwInvalidComplexType(const char *type)
{
    throw paramTypeError{
        "The passed value does not evaluate to a valid " + std::string(type) + " object",
        ""
    };
}

// A specific CFTypeValidationException subclass (message + optional detail) —
// carried as a paramTypeError.
[[noreturn]] void failValidation(const std::string &msg, const std::string &detail = "")
{
    throw paramTypeError{msg, detail};
}

// Whether a value can be cast to a string (CF's Cast._String throws for
// complex types, which surfaces as InvalidSimpleTypeException("string")).
bool isSimpleValue(const cfvariant *value)
{
    switch (value->m_type) {
    case cfvariant::Array:
    case cfvariant::Struct:
    case cfvariant::Query:
    case cfvariant::Xml:
    case cfvariant::Binary:
    case cfvariant::Image:
    case cfvariant::Component:
    case cfvariant::JSon:
    case cfvariant::Function:
        return false;
    default:
        return true;
    }
}

// ---- CF's validator classes (CFTypeValidatorFactory) ----

void validateArray(const cfvariant *value)
{
    if (value->m_type != cfvariant::Array) throwInvalidComplexType("array");
}

void validateBinary(const cfvariant *value)
{
    if (value->m_type != cfvariant::Binary) throwInvalidComplexType("binary");
}

void validateBoolean(const cfvariant *value)
{
    if (value->m_type == cfvariant::Boolean) return;
    if (value->m_type == cfvariant::Number || value->m_type == cfvariant::Long ||
        value->m_type == cfvariant::Float || value->m_type == cfvariant::DateTime) {
        return;
    }
    // Cast._boolean on a string: accepted values are yes/no/true/false/0/1.
    std::string s = safe_to_std_string(*value);
    std::string l = s;
    for (char &c : l) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    size_t b = 0, e = l.size();
    while (b < e && std::isspace(static_cast<unsigned char>(l[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(l[e - 1]))) e--;
    l = l.substr(b, e - b);
    if (l == "yes" || l == "no" || l == "true" || l == "false" || l == "0" || l == "1") return;
    throwInvalidSimpleType("boolean");
}

void validateNumeric(const cfvariant *value)
{
    if (!value) throwInvalidSimpleType("numeric");
    if (value->m_type == cfvariant::Boolean) {
        throwInvalidSimpleType("numeric");
    }
    std::string s = safe_to_std_string(*value);
    std::string l = s;
    for (char &c : l) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    size_t b = 0, e = l.size();
    while (b < e && std::isspace(static_cast<unsigned char>(l[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(l[e - 1]))) e--;
    std::string lt = l.substr(b, e - b);
    if (lt == "true" || lt == "false" || lt == "yes" || lt == "no") {
        // CFNumberValidator.checkIsBooleanString -> InvalidNumericTypeException.
        failValidation("The value cannot be converted to a numeric type.", "");
    }
    if (!cfmlIsValidNumeric(value)) throwInvalidSimpleType("numeric");
}

void validateInteger(const cfvariant *value)
{
    if (!value) throwInvalidSimpleType("integer");
    // CFIntegerValidator: a Number must have an integral double value; a
    // string must parse with Java's Integer.parseInt (int range).
    if (value->m_type == cfvariant::Number || value->m_type == cfvariant::Long ||
        value->m_type == cfvariant::Float) {
        double d = (value->m_type == cfvariant::Number) ? (double)value->m_int
                  : (value->m_type == cfvariant::Long) ? (double)value->m_long
                  : value->m_double;
        if (d == std::floor(d) && d >= -2147483648.0 && d <= 2147483647.0) return;
        failValidation(
            "The value specified, " + javaDoubleToString(d) + ", must be a valid integer.", "");
    }
    std::string s = safe_to_std_string(*value);
    long long v = 0;
    if (!cfmlStrictParseInt(s, v)) {
        failValidation("The value specified, " + s + ", must be a valid integer.", "");
    }
    if (v < -2147483648LL || v > 2147483647LL) {
        failValidation("The value specified, " + s + ", must be a valid integer.", "");
    }
}

void validateDate(const cfvariant *value)
{
    if (value->m_type == cfvariant::DateTime) return;
    double dummy = 0;
    if (parseDateTimeStr(const_cast<cfvariant*>(value)->toString(), dummy)) return;
    cfvariant *nd = cf_isnumericdate(value);
    if (nd->m_bool) return;
    throwInvalidSimpleType("date");
}

void validateString(const cfvariant *value)
{
    if (!isSimpleValue(value)) throwInvalidSimpleType("string");
}

void validateQuery(const cfvariant *value)
{
    if (value->m_type != cfvariant::Query) throwInvalidComplexType("query");
}

void validateStruct(const cfvariant *value)
{
    if (value->m_type != cfvariant::Struct) throwInvalidComplexType("struct");
}

void validateXml(const cfvariant *value)
{
    if (value->m_type != cfvariant::Xml) throwInvalidComplexType("xml");
}

void validateFunction(const cfvariant *value)
{
    if (value->m_type == cfvariant::Function || value->m_udf) return;
    failValidation("A valid UDF must be defined.", "");
}

void validateDateTimeObject(const cfvariant *value)
{
    if (value->m_type != cfvariant::DateTime) throwInvalidSimpleType("datetimeobject");
}

void validateGuid(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (!cfmlRegexFullMatch("[A-Fa-f0-9]{8,8}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{12,12}", s)) {
        failValidation("A string GUID value is required.",
            "A GUID is a string of length 36 formatted as XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX, "
            "where X is a hexadecimal digit (0-9 or A-F).");
    }
}

void validateUuid(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (!cfmlRegexFullMatch("[A-Fa-f0-9]{8,8}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{4,4}-[A-Fa-f0-9]{16,16}", s)) {
        failValidation("A string UUID value is required.",
            "A UUID is a string of length 35 formatted as XXXXXXXX-XXXX-XXXX-XXXXXXXXXXXXXXXX, "
            "where X is a hexadecimal digit (0-9 or A-F).");
    }
}

void validateSsn(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (!cfmlRegexFullMatch("^[0-9]{3}(-| )[0-9]{2}(-| )[0-9]{4}$", s)) {
        failValidation("A valid ssn is required.", "");
    }
}

void validateTelephone(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (!cfmlRegexFullMatch(
            "^(((1))?[ ,\\-,\\.]?([\\(]?([1-9][0-9]{2})[\\)]?))?[ ,\\-,\\.]?([^0-1]){1}([0-9]){2}[ ,\\-,\\.]?([0-9]){4}(( )((x){0,1}([0-9]){1,5}){0,1})?$",
            s)) {
        failValidation("A valid telephone is required.", "");
    }
}

void validateZipCode(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (!cfmlRegexFullMatch("^([0-9]){5,5}$|(([0-9]){5,5}(-| ){1}([0-9]){4,4}$)", s)) {
        failValidation("A valid US zipcode is required.", "");
    }
}

void validateUsDate(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (!cfmlIsUsDate(s)) throwInvalidSimpleType("date");
}

void validateEuroDate(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (!cfmlIsEuroDate(s)) {
        failValidation("A valid formatted eurodate must be defined.", "");
    }
}

void validateTime(const cfvariant *value)
{
    if (value->m_type == cfvariant::DateTime) return;
    std::string s = safe_to_std_string(*value);
    if (cfmlIsTimeString(s)) return;
    double dummy = 0;
    if (parseDateTimeStr(const_cast<cfvariant*>(value)->toString(), dummy)) return;
    failValidation("A valid formatted time must be defined.", "");
}

void validateCreditCard(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    std::string cc;
    for (char c : s) {
        if (std::isdigit(static_cast<unsigned char>(c))) cc += c;
    }
    if (cc.size() < 13 || cc.size() > 19) {
        failValidation("A valid credit card is required.", "");
    }
    double v = 0;
    if (!cfmlStrictParseDouble(cc, v)) {
        failValidation("A valid credit card is required.", "");
    }
    if (!cfmlLuhnCheck(cc)) {
        failValidation("A valid credit card is required.", "");
    }
}

void validateEmail(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (s.find(',') != std::string::npos) {
        failValidation("A valid e-mail is required.", "");
    }
    if (!cfmlRegexFullMatch("^[a-zA-Z_0-9-'\\+~]+(\\.[a-zA-Z_0-9-'\\+~]+)*@([a-zA-Z_0-9-]+\\.)+[a-zA-Z]{2,7}$", s)) {
        failValidation("A valid e-mail is required.", "");
    }
}

void validateVariableName(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    if (!cfmlIsValidVariableName(s)) {
        failValidation(
            (webstrada::string("Cannot set variable with name ") + webstrada::string(s.c_str()) +
                webstrada::string(".")).constData(),
            "The variable name is illegal. Variable names must start with a letter and can include "
            "only letters, numbers and underscores.");
    }
}

void validateUrl(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    std::string lv = s.substr(b, e - b);
    for (char &c : lv) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lv.find('[') != std::string::npos && lv.find(']') != std::string::npos) {
        if (cfmlRegexFullMatch(
                "^((http|https|ftp|file)\\:\\/\\/([a-zA-Z0-0]*:[a-zA-Z0-0]*(@))?(\\[)?[a-zA-Z0-9-\\.:]+(\\])?(\\.[a-zA-Z]{2,3})?(:[a-zA-Z0-9]*)?\\/?(([a-zA-Z0-9-\\._\\?\\,\\'\\/\\+&:@\\$#\\=~\\*\\!\\(\\)])|(%[a-f0-9]{2}))*)|((news)\\:[a-zA-Z0-9\\.]*)$",
                lv)) {
            return;
        }
        failValidation("A valid URL is required.", "");
    }
    if (!cfmlIsValidUrl(lv)) {
        failValidation("A valid URL is required.", "");
    }
}

void validateUrlIpv6(const cfvariant *value)
{
    std::string s = safe_to_std_string(*value);
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    std::string lv = s.substr(b, e - b);
    for (char &c : lv) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (!cfmlRegexFullMatch(
            "^((http|https|ftp|file)\\:\\/\\/([a-zA-Z0-0]*:[a-zA-Z0-0]*(@))?(\\[)?[a-zA-Z0-9-\\.:]+(\\])?(\\.[a-zA-Z]{2,3})?(:[a-zA-Z0-9]*)?\\/?(([a-zA-Z0-9-\\._\\?\\,\\'\\/\\+&:@\\$#\\=~\\*\\!\\(\\)])|(%[a-f0-9]{2}))*)|((news)\\:[a-zA-Z0-9\\.]*)$",
            lv)) {
        failValidation("A valid URL is required.", "");
    }
}

// ---- bracket-name helpers (name="arr[2]" / name="s[\"key\"]") ----

struct BracketName {
    std::string base;              // text before the first '['
    std::vector<std::string> idx;  // raw index texts (inside each [...] pair)
};

bool parseBracketName(const std::string &name, BracketName &out)
{
    size_t lb = name.find('[');
    if (lb == std::string::npos) return false;
    out.base = name.substr(0, lb);
    out.idx.clear();
    size_t pos = lb;
    while (pos < name.size() && name[pos] == '[') {
        size_t close = name.find(']', pos);
        if (close == std::string::npos) return false;
        out.idx.push_back(name.substr(pos + 1, close - pos - 1));
        pos = close + 1;
    }
    if (pos != name.size()) return false;
    return !out.base.empty() && !out.idx.empty();
}

// Evaluates a raw bracket index text: a numeric literal -> Number, a quoted
// string -> its key, a simple variable name -> its value (via lookupVarWritable),
// otherwise the raw text is used as a key.
cfvariant evalBracketIndex(const std::string &raw, void *cgi, void *server, void *cookie,
                           void *application, void *session, void *url, void *form, void *variables)
{
    std::string s = raw;
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
    std::string t = s.substr(b, e - b);
    if (t.size() >= 2 && (t.front() == '"' || t.front() == '\'') && t.back() == t.front()) {
        return cfvariant(t.substr(1, t.size() - 2).c_str());
    }
    long long iv = 0;
    if (cfmlStrictParseInt(t, iv)) {
        return cfvariant(static_cast<int>(iv));
    }
    bool isName = !t.empty();
    for (char c : t) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) { isName = false; break; }
    }
    if (isName) {
        cfvariant *v = lookupVarWritable(t.c_str(), cgi, server, cookie, application, session, url, form, variables);
        if (v) return *v;
    }
    return cfvariant(t.c_str());
}

// Non-mutating member navigation used by the "does the parameter exist?" check
// for bracket names: array bounds checked without creating rows, struct keys
// looked up without inserting a null slot.
cfvariant *navigateReadOnly(cfvariant *cur, const cfvariant &idx)
{
    if (!cur) return nullptr;
    if (cur->m_type == cfvariant::Array && !cur->m_queryColOwner) {
        int i = getIntValue(idx);
        if (i < 1 || i > (int)cur->m_array->size()) return nullptr;
        return &cur->m_array->at(i - 1);
    }
    if (cur->m_type == cfvariant::Struct) {
        webstrada::string key = const_cast<cfvariant*>(&idx)->toString();
        auto it = cur->m_struct->find(key);
        if (it == cur->m_struct->end()) return nullptr;
        return &it->second;
    }
    try {
        return cfvariant_index(cur, const_cast<cfvariant*>(&idx));
    } catch (...) {
        return nullptr;
    }
}

bool variantIsDefined(const cfvariant *v)
{
    return v && v->m_type != cfvariant::Null && v->m_type != cfvariant::NotSet && !v->m_disabled;
}
// Assigns the default value to a plain dotted/simple name (cfvariant_assign)
// or to a bracket name (auto-creating the base array/struct when absent).
cfvariant *assignDefault(const std::string &name, const cfvariant *defaultVal,
                         void *cgi, void *server, void *cookie,
                         void *application, void *session, void *url,
                         void *form, void *variables)
{
    BracketName bn;
    if (parseBracketName(name, bn)) {
        std::vector<const cfvariant*> chain;
        for (const auto &raw : bn.idx) {
            cfvariant *iv = new cfvariant(evalBracketIndex(raw, cgi, server, cookie, application, session, url, form, variables));
            cf_register_temp(iv);
            chain.push_back(iv);
        }
        cfvariant *base = lookupVarWritable(bn.base.c_str(), cgi, server, cookie, application, session, url, form, variables);
        if (!base) {
            // CF auto-creates the base collection: an array when the first
            // index is numeric, a struct otherwise (setAttribute("a[1]", v)).
            bool firstIsNumeric = false;
            {
                long long d = 0;
                std::string t = bn.idx[0];
                size_t b = 0, e = t.size();
                while (b < e && std::isspace(static_cast<unsigned char>(t[b]))) b++;
                while (e > b && std::isspace(static_cast<unsigned char>(t[e - 1]))) e--;
                std::string t2 = t.substr(b, e - b);
                if (t2.size() >= 2 && (t2.front() == '"' || t2.front() == '\'') && t2.back() == t2.front()) {
                    firstIsNumeric = false;
                } else {
                    firstIsNumeric = cfmlStrictParseInt(t2, d);
                }
            }
            cfvariant created(firstIsNumeric ? cfvariant::Array : cfvariant::Struct);
            cfvariant *newBase = cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                                                  static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                                                  static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                                                  static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                                                  bn.base.c_str(), &created);
            base = lookupVarWritable(bn.base.c_str(), cgi, server, cookie, application, session, url, form, variables);
            if (!base) return newBase;
        }
        cfvariant **chainArr = const_cast<cfvariant**>(chain.data());
        return cfvariant_index_assign_deep(base, const_cast<const cfvariant**>(chainArr), (int)chain.size(), defaultVal);
    }
    return cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                            static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                            static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                            static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                            name.c_str(), defaultVal);
}

} // namespace

cfvariant *cf_param(const cfvariant *name, const cfvariant *type,
                    const cfvariant *defaultVal, const cfvariant *min,
                    const cfvariant *max, const cfvariant *maxlength,
                    const cfvariant *pattern,
                    void *cgi, void *server, void *cookie,
                    void *application, void *session, void *url,
                    void *form, void *variables)
{
    std::string rawName = name ? safe_to_std_string(*name) : "";
    std::string pname = rawName;
    size_t b = 0, e = pname.size();
    while (b < e && std::isspace(static_cast<unsigned char>(pname[b]))) b++;
    while (e > b && std::isspace(static_cast<unsigned char>(pname[e - 1]))) e--;
    pname = pname.substr(b, e - b);

    // CF's two compile paths differ only in the missing-parameter error's name
    // casing: without a `type` attribute the checkSimpleParameter optimization
    // reports the canonical (uppercased) name; with `type` the full ParamTag
    // (_checkParam) reports the name exactly as written. Both are the same
    // runtime find-or-set behavior.
    bool upperName = (type == nullptr);

    // ---- find the parameter (findAttribute: variables + implicit scopes) ----
    cfvariant *found = nullptr;
    bool isBracket = false;
    BracketName bn;
    if (parseBracketName(pname, bn)) {
        isBracket = true;
        cfvariant *base = lookupVarWritable(bn.base.c_str(), cgi, server, cookie, application, session, url, form, variables);
        if (base) {
            found = base;
            for (const auto &raw : bn.idx) {
                cfvariant idxVal = evalBracketIndex(raw, cgi, server, cookie, application, session, url, form, variables);
                found = navigateReadOnly(found, idxVal);
                if (!found || !variantIsDefined(found)) { found = nullptr; break; }
            }
        }
    } else {
        bool savedImplicit = g_searchImplicitScopes;
        g_searchImplicitScopes = true;   // cfparam finds URL/FORM/COOKIE/CGI names
        found = lookupVarWritable(pname.c_str(), cgi, server, cookie, application, session, url, form, variables);
        g_searchImplicitScopes = savedImplicit;
    }

    if (!found || !variantIsDefined(found)) {
        if (!defaultVal) {
            throwParamNotFound(pname, upperName);
        }
        cfvariant *assigned = assignDefault(pname, defaultVal, cgi, server, cookie, application, session, url, form, variables);
        found = assigned;
    }

    cfvariant value = *found;

    // ---- type validation (ParamTag.doStartTag) ----
    if (!type) return new cfvariant(value);
    std::string t = safe_to_std_string(*type);
    size_t tb = 0, te = t.size();
    while (tb < te && std::isspace(static_cast<unsigned char>(t[tb]))) tb++;
    while (te > tb && std::isspace(static_cast<unsigned char>(t[te - 1]))) te--;
    std::string ttrim = t.substr(tb, te - tb);
    std::string tlow = ttrim;
    for (char &c : tlow) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (tlow != "any") {
        std::string pat = pattern ? safe_to_std_string(*pattern) : "";

        if (tlow == "regex" && !pat.empty()) {
            std::string subject = safe_to_std_string(value);
            if (!cfmlRegexFullMatch(pat, subject)) {
                throwInvalidParamType("The value does not match the regular expression pattern provided.", "");
            }
            return new cfvariant(value);
        }
        if (tlow == "range" && (min != nullptr || max != nullptr)) {
            double lo = min ? getDoubleValue(*const_cast<cfvariant*>(min)) : 0.0;
            double hi = max ? getDoubleValue(*const_cast<cfvariant*>(max)) : 0.0;
            double v = 0;
            std::string vs = safe_to_std_string(value);
            if (!cfmlStrictParseDouble(vs, v)) throwInvalidSimpleType("numeric");
            if (min && v < lo) {
                throwInvalidParamType(
                    (webstrada::string("The value specified, ") + webstrada::string(javaDoubleToString(v).c_str()) +
                        webstrada::string(", must be greater than or equal to ") + webstrada::string(javaDoubleToString(lo).c_str()) +
                        webstrada::string(".")).constData(),
                    "");
            }
            if (max && v > hi) {
                throwInvalidParamType(
                    (webstrada::string("The value specified, ") + webstrada::string(javaDoubleToString(v).c_str()) +
                        webstrada::string(", must be smaller than or equal to ") + webstrada::string(javaDoubleToString(hi).c_str()) +
                        webstrada::string(".")).constData(),
                    "");
            }
            return new cfvariant(value);
        }

        // maxlength applies to string/email/url only (CF's ParamTag; the raw
        // CFStringValidator exception propagates unwrapped, with the Java class
        // as its type).
        int ml = 0;
        if (maxlength) {
            std::string mlStr = safe_to_std_string(*maxlength);
            long long mlv = 0;
            if (cfmlStrictParseInt(mlStr, mlv)) ml = (int)mlv;
        }
        if (ml != 0 && (tlow == "string" || tlow == "email" || tlow == "url")) {
            if (!isSimpleValue(&value)) throwInvalidSimpleType("string");
            int len = (int)const_cast<cfvariant*>(&value)->toString().length();
            if (len > ml) {
                throw webstrada::exception(
                    webstrada::string("coldfusion.tagext.validation.CFStringValidator$StringUpperBoundException"),
                    webstrada::string("The length of the string, ") + webstrada::string::number(len) +
                        webstrada::string(" character(s), must be less than or equal to ") +
                        webstrada::string::number(ml) + webstrada::string(" character(s)."),
                    webstrada::string());
            }
        }

        // The general validator for the trimmed type name. A CFTypeValidator
        // failure is caught here and wrapped in CF's InvalidParamTypeException
        // (message "Invalid parameter type.", detail = validator msg+detail).
        try {
            const cfvariant *vptr = &value;
            if (tlow == "array") validateArray(vptr);
            else if (tlow == "binary") validateBinary(vptr);
            else if (tlow == "boolean") validateBoolean(vptr);
            else if (tlow == "date") validateDate(vptr);
            else if (tlow == "datetime_object" || tlow == "datetimeobject") validateDateTimeObject(vptr);
            else if (tlow == "email") validateEmail(vptr);
            else if (tlow == "eurodate") validateEuroDate(vptr);
            else if (tlow == "float" || tlow == "numeric") validateNumeric(vptr);
            else if (tlow == "function") validateFunction(vptr);
            else if (tlow == "guid") validateGuid(vptr);
            else if (tlow == "integer") validateInteger(vptr);
            else if (tlow == "numeric_legacy") validateNumeric(vptr);
            else if (tlow == "query") validateQuery(vptr);
            else if (tlow == "creditcard") validateCreditCard(vptr);
            else if (tlow == "regex") {
                // type="regex" without a pattern: CFRegExValidator.validate throws
                // a raw IllegalStateException (no pattern compiled).
                throw webstrada::exception(webstrada::string("java.lang.IllegalStateException"), webstrada::string(), webstrada::string());
            }
            else if (tlow == "ssn" || tlow == "social_security_number") validateSsn(vptr);
            else if (tlow == "string") validateString(vptr);
            else if (tlow == "struct") validateStruct(vptr);
            else if (tlow == "telephone") validateTelephone(vptr);
            else if (tlow == "time") validateTime(vptr);
            else if (tlow == "usdate") validateUsDate(vptr);
            else if (tlow == "uuid") validateUuid(vptr);
            else if (tlow == "url") validateUrl(vptr);
            else if (tlow == "urlipv6") validateUrlIpv6(vptr);
            else if (tlow == "variablename") validateVariableName(vptr);
            else if (tlow == "zipcode") validateZipCode(vptr);
            else if (tlow == "xml") validateXml(vptr);
            else if (tlow == "typed_array") {
                // CF's CFTypedArrayValidator is not usable as a plain type.
                throw webstrada::exception(webstrada::string("java.lang.UnsupportedOperationException"),
                                          webstrada::string(), webstrada::string());
            }
            else throwIllegalParamType();
        } catch (const paramTypeError &e) {
            throwInvalidParamType(e.msg, e.detail);
        }
    }

    return new cfvariant(value);
}

void cf_objectcache(const cfvariant *action)
{
    std::string a = action ? safe_to_std_string(*action) : "";
    std::string al = a;
    for (char &c : al) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (al == "clear") {
        webstrada::cache_store().removeAll("QUERY");
        return;
    }
    std::string shown = a.empty() ? "''" : a;
    throw webstrada::exception(webstrada::string("Template"),
        webstrada::string("Attribute validation error for CFOBJECTCACHE."),
        webstrada::string("The value of the ACTION attribute, which is currently ") +
            webstrada::string(shown.c_str()) + webstrada::string(", must be one of the values: CLEAR."));
}

} // namespace cfml

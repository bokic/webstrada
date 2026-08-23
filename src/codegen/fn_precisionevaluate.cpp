/**
 * @file fn_precisionevaluate.cpp
 * @brief CFML PrecisionEvaluate() built-in.
 *
 * Evaluates string expressions with Java BigDecimal-style arbitrary-precision
 * decimal arithmetic, reproducing CF 2025's PrecisionEvaluate exactly (verified
 * byte-for-byte against the RDS host and local Java):
 *
 *  - + - * use exact decimal arithmetic (add keeps max(a,b).scale; multiply
 *    keeps a.scale+b.scale; operands parse like Cast._BigDecimal).
 *  - / implements CfJspPage._divideBD: first a.divide(b, ROUND_UNNECESSARY)
 *    (result scale = a.scale; fails when the exact quotient needs more
 *    fractional digits than a.scale), falling back to a.divide(b, 20,
 *    HALF_EVEN) — scale 20, round-half-even.
 *  - ^ implements _powBD: new BigDecimal(Math.pow(a.doubleValue(),
 *    b.doubleValue())) — plain double math converted to the exact decimal
 *    expansion of the double (BigDecimal(double) semantics).
 *  - MOD / % and \ use integer arithmetic (truncate operands toward zero),
 *    like the imod / idiv bytecode.
 *  - Results render with BigDecimal.toString() (plain vs E notation).
 *
 * The expression is parsed with the engine's real CFML expression parser:
 * the argument is wrapped in `component { <expr> }` (a .cfc script-form
 * component), tokenized with textparser, and turned into an ExprAST via
 * parseTokensToAST; a BigDecimal-aware evaluator walks that AST. This lives in
 * the compiler library because that is where textparser + parseTokensToAST
 * (and the cfml_definition) are available; both the JIT and the interpreter
 * dispatch to it. CF's precision grammar rejects the symbolic < > <= >=
 * comparison operators (only == != and the word forms work), reproduced here
 * by scanning the operator tokens.
 */

#include "codegen_internal.h"

#include <textparser.hpp>
#include <cfml_definition.json.h>

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cfml {

namespace {

// ---------------------------------------------------------------------------
// CfBigDecimal: value = ±digits × 10^-scale
// ---------------------------------------------------------------------------

struct CfBigDecimal {
    bool neg = false;
    std::string digits = "0"; // significant digits, no leading zeros; "0" for zero
    int scale = 0;            // may be negative (e-notation literals)
};

int cmpDigits(const std::string &a, const std::string &b)
{
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    if (a == b) return 0;
    return a < b ? -1 : 1;
}

std::string stripLeadingZeros(std::string s)
{
    size_t i = 0;
    while (i + 1 < s.size() && s[i] == '0') i++;
    return s.substr(i);
}

std::string addDigits(const std::string &a, const std::string &b)
{
    std::string out;
    int carry = 0;
    size_t i = a.size(), j = b.size();
    while (i > 0 || j > 0 || carry) {
        int da = i > 0 ? a[--i] - '0' : 0;
        int db = j > 0 ? b[--j] - '0' : 0;
        int sum = da + db + carry;
        out.push_back(static_cast<char>('0' + sum % 10));
        carry = sum / 10;
    }
    std::reverse(out.begin(), out.end());
    return stripLeadingZeros(out);
}

// a - b, requires a >= b.
std::string subDigits(const std::string &a, const std::string &b)
{
    std::string out;
    int borrow = 0;
    size_t i = a.size(), j = b.size();
    while (i > 0) {
        int da = a[--i] - '0';
        int db = j > 0 ? b[--j] - '0' : 0;
        int d = da - db - borrow;
        if (d < 0) { d += 10; borrow = 1; } else { borrow = 0; }
        out.push_back(static_cast<char>('0' + d));
    }
    std::reverse(out.begin(), out.end());
    return stripLeadingZeros(out);
}

std::string mulDigits(const std::string &a, const std::string &b)
{
    if (a == "0" || b == "0") return "0";
    std::vector<int> res(a.size() + b.size(), 0);
    for (size_t i = a.size(); i-- > 0;) {
        for (size_t j = b.size(); j-- > 0;) {
            int p = (a[i] - '0') * (b[j] - '0') + res[i + j + 1];
            res[i + j + 1] = p % 10;
            res[i + j] += p / 10;
        }
    }
    std::string out;
    bool started = false;
    for (int v : res) {
        if (v != 0) started = true;
        if (started) out.push_back(static_cast<char>('0' + v));
    }
    return out.empty() ? "0" : out;
}

std::string mulPow10(const std::string &a, int k)
{
    if (a == "0") return "0";
    std::string out = a;
    out.append(static_cast<size_t>(k), '0');
    return out;
}

std::string mulSmall(const std::string &a, int n)
{
    if (n == 0 || a == "0") return "0";
    std::string out;
    int carry = 0;
    for (size_t i = a.size(); i-- > 0;) {
        int p = (a[i] - '0') * n + carry;
        out.push_back(static_cast<char>('0' + p % 10));
        carry = p / 10;
    }
    while (carry) {
        out.push_back(static_cast<char>('0' + carry % 10));
        carry /= 10;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

std::string mulSmallPow(const std::string &a, int base, int k)
{
    std::string out = a;
    for (int i = 0; i < k; i++) out = mulSmall(out, base);
    return out;
}

// Long division of digit strings (b != "0"): q = a/b, r = a%b.
void divMod(const std::string &a, const std::string &b, std::string &q, std::string &r)
{
    q.clear();
    std::string rem = "0";
    for (char c : a) {
        rem = (rem == "0") ? std::string(1, c) : rem + c;
        rem = stripLeadingZeros(rem);
        int d = 0;
        while (cmpDigits(rem, b) >= 0) { rem = subDigits(rem, b); d++; }
        q.push_back(static_cast<char>('0' + d));
    }
    q = stripLeadingZeros(q);
    r = rem;
}

bool isZeroBd(const CfBigDecimal &v) { return v.digits == "0"; }

// Parse a CFML number literal (sign, digits, '.', [eE][+-]exp).
CfBigDecimal parseBigDecimal(const std::string &s0)
{
    std::string s = s0;
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t')) b++;
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t')) e--;
    s = s.substr(b, e - b);

    bool neg = false;
    size_t i = 0;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) { neg = (s[i] == '-'); i++; }
    std::string intPart, fracPart;
    bool sawDot = false, sawDigit = false;
    for (; i < s.size(); i++) {
        char c = s[i];
        if (c == '.') { if (sawDot) break; sawDot = true; continue; }
        if (c >= '0' && c <= '9') {
            sawDigit = true;
            if (!sawDot) intPart += c; else fracPart += c;
            continue;
        }
        break;
    }
    if (!sawDigit) {
        throw webstrada::exception(("The value " + s + " cannot be converted to a number.").c_str());
    }
    long long exp = 0;
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        bool eneg = false;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) { eneg = (s[i] == '-'); i++; }
        long long ev = 0;
        bool edig = false;
        for (; i < s.size(); i++) {
            if (s[i] < '0' || s[i] > '9') break;
            ev = ev * 10 + (s[i] - '0');
            edig = true;
        }
        if (!edig) {
            throw webstrada::exception(("The value " + s + " cannot be converted to a number.").c_str());
        }
        exp = eneg ? -ev : ev;
    }
    CfBigDecimal out;
    out.digits = stripLeadingZeros(intPart + fracPart);
    out.scale = static_cast<int>(static_cast<long long>(fracPart.size()) - exp);
    out.neg = neg && out.digits != "0";
    return out;
}

// Exact conversion of a double to the decimal that its binary value equals
// (BigDecimal(double) constructor semantics).
CfBigDecimal bdFromDouble(double d)
{
    if (d == 0.0) {
        CfBigDecimal z;
        z.neg = std::signbit(d);
        return z;
    }
    if (!std::isfinite(d)) {
        throw webstrada::exception("NumberFormatException: Infinite or NaN");
    }
    uint64_t bits;
    std::memcpy(&bits, &d, sizeof(bits));
    int sign = (bits >> 63) ? -1 : 1;
    int expField = static_cast<int>((bits >> 52) & 0x7ff);
    uint64_t sig = (expField == 0)
        ? (bits & 0xfffffffffffffULL) << 1
        : (bits & 0xfffffffffffffULL) | 0x10000000000000ULL;
    long long exponent = static_cast<long long>(expField) - 1075;
    while (sig != 0 && (sig & 1) == 0) { sig >>= 1; exponent++; }

    CfBigDecimal out;
    out.neg = sign < 0;
    std::string digits = std::to_string(sig);
    if (exponent == 0) {
        out.digits = digits;
        out.scale = 0;
    } else if (exponent < 0) {
        out.digits = mulSmallPow(digits, 5, static_cast<int>(-exponent));
        out.scale = static_cast<int>(-exponent);
    } else {
        out.digits = mulSmallPow(digits, 2, static_cast<int>(exponent));
        out.scale = 0;
    }
    return out;
}

double bdToDouble(const CfBigDecimal &v)
{
    std::string s;
    if (v.neg) s += '-';
    if (v.scale <= 0) {
        s += v.digits;
        s.append(static_cast<size_t>(-v.scale), '0');
    } else if (static_cast<size_t>(v.scale) < v.digits.size()) {
        size_t cut = v.digits.size() - static_cast<size_t>(v.scale);
        s += v.digits.substr(0, cut) + "." + v.digits.substr(cut);
    } else {
        s += "0.";
        s.append(static_cast<size_t>(v.scale) - v.digits.size(), '0');
        s += v.digits;
    }
    return strtod(s.c_str(), nullptr);
}

// BigDecimal.toString() semantics.
std::string bdToString(const CfBigDecimal &v)
{
    std::string out;
    if (v.neg) out += '-';
    const std::string &d = v.digits;
    int s = v.scale;
    if (s <= 0) {
        if (s == 0) { out += d; return out; }
        long long adjExp = static_cast<long long>(d.size()) - s - 1;
        if (d.size() == 1) out += d;
        else out += d.substr(0, 1) + "." + d.substr(1);
        out += 'E';
        out += (adjExp >= 0) ? "+" : "-";
        out += std::to_string(adjExp < 0 ? -adjExp : adjExp);
        return out;
    }
    int precision = static_cast<int>(d.size());
    long long adjExp = static_cast<long long>(precision) - s - 1;
    if (adjExp >= -6) {
        if (precision > s) {
            out += d.substr(0, static_cast<size_t>(precision - s));
            out += '.';
            out += d.substr(static_cast<size_t>(precision - s));
        } else {
            out += "0.";
            out.append(static_cast<size_t>(s - precision), '0');
            out += d;
        }
        return out;
    }
    if (d.size() == 1) out += d;
    else out += d.substr(0, 1) + "." + d.substr(1);
    out += 'E';
    out += (adjExp >= 0) ? "+" : "-";
    out += std::to_string(adjExp < 0 ? -adjExp : adjExp);
    return out;
}

CfBigDecimal bdAdd(const CfBigDecimal &a, const CfBigDecimal &b)
{
    int sc = std::max(a.scale, b.scale);
    std::string da = a.digits, db = b.digits;
    da.append(static_cast<size_t>(sc - a.scale), '0');
    db.append(static_cast<size_t>(sc - b.scale), '0');
    CfBigDecimal out;
    out.scale = sc;
    if (a.neg == b.neg) {
        out.digits = addDigits(da, db);
        out.neg = a.neg && out.digits != "0";
    } else {
        int c = cmpDigits(da, db);
        if (c == 0) { out.digits = "0"; out.neg = false; }
        else if (c > 0) { out.digits = subDigits(da, db); out.neg = a.neg; }
        else { out.digits = subDigits(db, da); out.neg = b.neg; }
    }
    return out;
}

CfBigDecimal bdNegate(const CfBigDecimal &a)
{
    CfBigDecimal r = a;
    if (!isZeroBd(a)) r.neg = !r.neg;
    return r;
}

CfBigDecimal bdSub(const CfBigDecimal &a, const CfBigDecimal &b) { return bdAdd(a, bdNegate(b)); }

CfBigDecimal bdMul(const CfBigDecimal &a, const CfBigDecimal &b)
{
    CfBigDecimal out;
    out.digits = mulDigits(a.digits, b.digits);
    out.scale = a.scale + b.scale;
    out.neg = (a.neg != b.neg) && out.digits != "0";
    return out;
}

// Divide a/b rounding to `targetScale`. With `roundHalfEven` false this is the
// UNNECESSARY attempt (fails when the quotient needs rounding); with it true it
// rounds half-even. Returns true on success and sets `out`.
bool divideToScale(const CfBigDecimal &a, const CfBigDecimal &b, int targetScale,
                   bool roundHalfEven, CfBigDecimal &out)
{
    if (isZeroBd(a)) {
        out.digits = "0";
        out.scale = targetScale;
        out.neg = false;
        return true;
    }
    long long kll = static_cast<long long>(targetScale) - a.scale + b.scale;
    std::string dividend = a.digits, divisor = b.digits;
    if (kll >= 0) dividend = mulPow10(a.digits, static_cast<int>(kll));
    else divisor = mulPow10(b.digits, static_cast<int>(-kll));
    std::string q, r;
    divMod(dividend, divisor, q, r);
    if (r != "0") {
        if (!roundHalfEven) return false;
        std::string twice = mulSmall(r, 2);
        int c = cmpDigits(twice, divisor);
        bool roundUp = false;
        if (c > 0) roundUp = true;
        else if (c == 0) roundUp = ((q.back() - '0') % 2) == 1;
        if (roundUp) q = addDigits(q, "1");
    }
    out.digits = q;
    out.scale = targetScale;
    out.neg = (a.neg != b.neg) && q != "0";
    return true;
}

// CfJspPage._divideBD semantics.
CfBigDecimal bdDivide(const CfBigDecimal &a, const CfBigDecimal &b)
{
    if (isZeroBd(b)) throw webstrada::exception("Division by zero.");
    CfBigDecimal out;
    if (divideToScale(a, b, a.scale, false, out)) return out;
    divideToScale(a, b, 20, true, out);
    return out;
}

// _powBD: new BigDecimal(Math.pow(a.doubleValue(), b.doubleValue())).
CfBigDecimal bdPow(const CfBigDecimal &a, const CfBigDecimal &b)
{
    double r = std::pow(bdToDouble(a), bdToDouble(b));
    return bdFromDouble(r);
}

int bdCompare(const CfBigDecimal &a, const CfBigDecimal &b)
{
    if (a.neg != b.neg) return a.neg ? -1 : 1;
    bool za = isZeroBd(a), zb = isZeroBd(b);
    if (za && zb) return 0;
    int sc = std::max(a.scale, b.scale);
    std::string da = a.digits, db = b.digits;
    da.append(static_cast<size_t>(sc - a.scale), '0');
    db.append(static_cast<size_t>(sc - b.scale), '0');
    int c = cmpDigits(da, db);
    if (a.neg) c = -c;
    return c;
}

std::string truncIntDigits(const CfBigDecimal &v)
{
    std::string d = v.digits;
    if (v.scale <= 0) return mulPow10(d, -v.scale);
    if (static_cast<size_t>(v.scale) < d.size()) {
        return d.substr(0, d.size() - static_cast<size_t>(v.scale));
    }
    return "0";
}

// ---------------------------------------------------------------------------
// BigDecimal AST evaluator
// ---------------------------------------------------------------------------

// cfvariant -> std::string (local helper; the cftags common.h safe_to_std_string
// is not part of the compiler library).
std::string varToStdString(const cfvariant &v)
{
    const webstrada::string s = const_cast<cfvariant&>(v).toString();
    const char *p = s.constData();
    return p ? std::string(p) : std::string();
}

struct PeValue {
    enum Kind { Number, Bool, Str } kind = Number;
    CfBigDecimal num;
    bool b = false;
    std::string str;
};

// Build a Float cfvariant carrying the exact decimal text.
cfvariant *bdResult(const CfBigDecimal &bd)
{
    auto *ret = new cfvariant(cfvariant::Float);
    ret->m_double = bdToDouble(bd);
    ret->m_literalText = new string(bdToString(bd).c_str());
    return ret;
}

cfvariant *finish(const PeValue &v)
{
    if (v.kind == PeValue::Bool) {
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = v.b;
        ret->m_boolLiteral = false;
        return ret;
    }
    if (v.kind == PeValue::Str) return new cfvariant(v.str.c_str());
    return bdResult(v.num);
}

bool truthy(const PeValue &v)
{
    if (v.kind == PeValue::Bool) return v.b;
    if (v.kind == PeValue::Number) return !isZeroBd(v.num);
    return !v.str.empty();
}

std::string stringify(const PeValue &v)
{
    if (v.kind == PeValue::Number) return bdToString(v.num);
    if (v.kind == PeValue::Bool) return v.b ? "YES" : "NO";
    return v.str;
}

bool toNumber(PeValue &v)
{
    if (v.kind == PeValue::Number) return true;
    if (v.kind == PeValue::Str) {
        try {
            v.num = parseBigDecimal(v.str);
            v.kind = PeValue::Number;
            return true;
        } catch (const webstrada::exception &) {
            return false;
        }
    }
    return false;
}

// Convert a resolved cfvariant to a PeValue (Cast._BigDecimal via toString).
PeValue fromVariant(const cfvariant *val)
{
    PeValue v;
    if (!val) { v.kind = PeValue::Str; return v; }
    if (val->m_type == cfvariant::Boolean) {
        v.kind = PeValue::Bool;
        v.b = val->m_bool;
    } else if (val->m_type == cfvariant::String) {
        v.kind = PeValue::Str;
        v.str = varToStdString(*val);
    } else if (val->m_type == cfvariant::Number || val->m_type == cfvariant::Long ||
               val->m_type == cfvariant::Float) {
        v.kind = PeValue::Number;
        v.num = parseBigDecimal(varToStdString(*val));
    } else {
        v.kind = PeValue::Number;
        v.num = parseBigDecimal(varToStdString(*val));
    }
    return v;
}

struct EvalCtx {
    const char *text;
    void *cgi, *server, *cookie, *application, *session, *url, *form, *variables;
};

PeValue evalPrecision(const ExprAST &node, const EvalCtx &ctx);

PeValue intDiv(const PeValue &a, const PeValue &b)
{
    PeValue out;
    out.kind = PeValue::Number;
    if (isZeroBd(b.num)) throw webstrada::exception("Division by zero.");
    std::string da = truncIntDigits(a.num);
    std::string db = truncIntDigits(b.num);
    if (db == "0") throw webstrada::exception("Division by zero.");
    std::string q, r;
    divMod(da, db, q, r);
    out.num.digits = q;
    out.num.scale = 0;
    out.num.neg = (a.num.neg != b.num.neg) && q != "0";
    return out;
}

PeValue intMod(const PeValue &a, const PeValue &b)
{
    PeValue out;
    out.kind = PeValue::Number;
    if (isZeroBd(b.num)) throw webstrada::exception("Division by zero.");
    std::string da = truncIntDigits(a.num);
    std::string db = truncIntDigits(b.num);
    if (db == "0") throw webstrada::exception("Division by zero.");
    std::string q, r;
    divMod(da, db, q, r);
    out.num.digits = r;
    out.num.scale = 0;
    out.num.neg = a.num.neg && r != "0";
    return out;
}

PeValue evalPrecision(const ExprAST &node, const EvalCtx &ctx)
{
    switch (node.type) {
    case ExprAST::LiteralInt: {
        PeValue v;
        v.kind = PeValue::Number;
        v.num = parseBigDecimal(std::to_string(node.int_val));
        return v;
    }
    case ExprAST::LiteralLong: {
        PeValue v;
        v.kind = PeValue::Number;
        v.num = parseBigDecimal(std::to_string(node.long_val));
        return v;
    }
    case ExprAST::LiteralFloat: {
        PeValue v;
        v.kind = PeValue::Number;
        if (!node.float_literal_text.empty()) {
            v.num = parseBigDecimal(node.float_literal_text);
        } else {
            v.num = bdFromDouble(node.float_val);
        }
        return v;
    }
    case ExprAST::LiteralBool: {
        PeValue v;
        v.kind = PeValue::Bool;
        v.b = node.bool_val;
        return v;
    }
    case ExprAST::LiteralString: {
        PeValue v;
        v.kind = PeValue::Str;
        const auto &tok = node.token;
        std::string text;
        if (ctx.text && tok.len >= 2) {
            text.assign(ctx.text + tok.position, tok.len);
            char q = text.front();
            text = text.substr(1, text.size() - 2);
            // CFML: a doubled quote inside a quoted literal is an escaped quote.
            std::string esc(2, q);
            size_t pos;
            while ((pos = text.find(esc)) != std::string::npos) {
                text.replace(pos, 2, std::string(1, q));
            }
        }
        v.str = text;
        return v;
    }
    case ExprAST::Variable: {
        cfvariant *val = cfvariant_bare_identifier(
            static_cast<const cfvariant*>(ctx.cgi), static_cast<const cfvariant*>(ctx.server),
            static_cast<const cfvariant*>(ctx.cookie), static_cast<const cfvariant*>(ctx.application),
            static_cast<const cfvariant*>(ctx.session), static_cast<const cfvariant*>(ctx.url),
            static_cast<const cfvariant*>(ctx.form), static_cast<cfvariant*>(ctx.variables),
            node.string_val.c_str());
        return fromVariant(val);
    }
    case ExprAST::UnaryOp: {
        PeValue v = evalPrecision(*node.right, ctx);
        if (node.op_val == "+") {
            return v;
        }
        if (node.op_val == "-") {
            if (v.kind == PeValue::Number) { v.num = bdNegate(v.num); return v; }
            if (toNumber(v)) { v.num = bdNegate(v.num); return v; }
            throw webstrada::exception("Invalid CFML construct found.");
        }
        // NOT / !
        PeValue out;
        out.kind = PeValue::Bool;
        out.b = !truthy(v);
        return out;
    }
    case ExprAST::TernaryExpr: {
        PeValue cond = evalPrecision(*node.left, ctx);
        if (truthy(cond)) {
            return evalPrecision(*node.right, ctx);
        }
        return evalPrecision(*node.args[0], ctx);
    }
    case ExprAST::BinaryOp: {
        const std::string &op = node.op_val;
        if (op == ".") {
            // Member access a.b.c: reconstruct the dotted path (parseTokensToAST
            // builds BinaryOp(".") trees) and resolve it like _autoscalarize.
            std::string path;
            const ExprAST *cur = &node;
            while (cur && cur->type == ExprAST::BinaryOp && cur->op_val == ".") {
                if (!cur->right || cur->right->type != ExprAST::Variable) {
                    throw webstrada::exception("PrecisionEvaluate does not support this expression.");
                }
                path = (path.empty() ? cur->right->string_val : cur->right->string_val + "." + path);
                cur = cur->left.get();
            }
            if (!cur || cur->type != ExprAST::Variable) {
                throw webstrada::exception("PrecisionEvaluate does not support this expression.");
            }
            path = (path.empty() ? cur->string_val : cur->string_val + "." + path);
            cfvariant *val = cfvariant_bare_identifier(
                static_cast<const cfvariant*>(ctx.cgi), static_cast<const cfvariant*>(ctx.server),
                static_cast<const cfvariant*>(ctx.cookie), static_cast<const cfvariant*>(ctx.application),
                static_cast<const cfvariant*>(ctx.session), static_cast<const cfvariant*>(ctx.url),
                static_cast<const cfvariant*>(ctx.form), static_cast<cfvariant*>(ctx.variables),
                path.c_str());
            return fromVariant(val);
        }
        if (op == "&") {
            PeValue l = evalPrecision(*node.left, ctx);
            PeValue r = evalPrecision(*node.right, ctx);
            PeValue out;
            out.kind = PeValue::Str;
            out.str = stringify(l) + stringify(r);
            return out;
        }
        if (op == "?" || op == ":") {
            throw webstrada::exception("Invalid CFML construct found.");
        }
        if (op == "AND" || op == "OR" || op == "XOR" || op == "EQV" || op == "IMP") {
            PeValue l = evalPrecision(*node.left, ctx);
            PeValue r = evalPrecision(*node.right, ctx);
            bool x = truthy(l), y = truthy(r);
            PeValue out;
            out.kind = PeValue::Bool;
            if (op == "AND") out.b = x && y;
            else if (op == "OR") out.b = x || y;
            else if (op == "XOR") out.b = x != y;
            else if (op == "EQV") out.b = x == y;
            else out.b = !(x && !y); // IMP
            return out;
        }
        if (op == "EQ" || op == "NEQ" || op == "IS" || op == "IS NOT" ||
            op == "LT" || op == "LTE" || op == "LE" || op == "GT" || op == "GTE" || op == "GE" ||
            op == "==" || op == "!=" ||
            op == "CONTAINS" || op == "DOES NOT CONTAIN") {
            PeValue l = evalPrecision(*node.left, ctx);
            PeValue r = evalPrecision(*node.right, ctx);
            bool aNum = toNumber(l);
            bool bNum = toNumber(r);
            int c = (aNum && bNum) ? bdCompare(l.num, r.num) : stringify(l).compare(stringify(r));
            PeValue out;
            out.kind = PeValue::Bool;
            if (op == "EQ" || op == "IS" || op == "==") out.b = (c == 0);
            else if (op == "NEQ" || op == "IS NOT" || op == "!=") out.b = (c != 0);
            else if (op == "LT") out.b = (c < 0);
            else if (op == "LTE" || op == "LE") out.b = (c <= 0);
            else if (op == "GT") out.b = (c > 0);
            else if (op == "GTE" || op == "GE") out.b = (c >= 0);
            else if (op == "CONTAINS") out.b = stringify(l).find(stringify(r)) != std::string::npos;
            else out.b = stringify(l).find(stringify(r)) == std::string::npos; // DOES NOT CONTAIN
            return out;
        }
        // Arithmetic: + - * / \ % MOD ^
        PeValue l = evalPrecision(*node.left, ctx);
        PeValue r = evalPrecision(*node.right, ctx);
        if (op == "+") {
            if (toNumber(l) && toNumber(r)) { l.num = bdAdd(l.num, r.num); return l; }
            PeValue out; out.kind = PeValue::Str; out.str = stringify(l) + stringify(r); return out;
        }
        if (!toNumber(l) || !toNumber(r)) throw webstrada::exception("Invalid CFML construct found.");
        if (op == "-") { l.num = bdSub(l.num, r.num); return l; }
        if (op == "*") { l.num = bdMul(l.num, r.num); return l; }
        if (op == "/") { l.num = bdDivide(l.num, r.num); return l; }
        if (op == "\\") return intDiv(l, r);
        if (op == "%" || op == "MOD") return intMod(l, r);
        if (op == "^") { l.num = bdPow(l.num, r.num); return l; }
        throw webstrada::exception("Invalid CFML construct found.");
    }
    default:
        throw webstrada::exception("PrecisionEvaluate does not support this expression.");
    }
}

// ---------------------------------------------------------------------------
// Tokenization: wrap the expression in `component { <expr> }` (a .cfc script
// component) and extract the inner ScriptExpression's tokens.
// ---------------------------------------------------------------------------

bool tokenizePrecisionExpr(const std::string &expr, std::string &wrappedText,
                           std::vector<TextParserTokenItem> &outTokens)
{
    wrappedText = "component { " + expr + " }";
    textparser_t h = nullptr;
    if (textparser_openmem(wrappedText.c_str(), static_cast<int>(wrappedText.size()),
                           TEXTPARSER_ENCODING_UTF_8, &h) != 0) {
        return false;
    }
    textparser_set_filename(h, "precisionevaluate.cfc");
    if (textparser_parse(h, &cfml_definition) != 0) {
        textparser_close(h);
        return false;
    }

    bool ok = false;
    for (const textparser_token_item *tok = textparser_get_first_token(h); tok && !ok;
         tok = textparser_get_token_next(tok)) {
        if (tok->token_id != TextParser_cfml_ScriptExpression) continue;
        size_t n = textparser_get_token_children_count(tok);
        const textparser_token_item *child = textparser_get_token_child(tok);
        for (size_t i = 0; i < n && !ok; i++, child = textparser_get_token_next(child)) {
            if (child->token_id != TextParser_cfml_CodeBlock) continue;
            size_t cn = textparser_get_token_children_count(child);
            const textparser_token_item *cchild = textparser_get_token_child(child);
            for (size_t j = 0; j < cn; j++, cchild = textparser_get_token_next(cchild)) {
                if (cchild->token_id != TextParser_cfml_ScriptExpression) continue;
                size_t en = textparser_get_token_children_count(cchild);
                const textparser_token_item *etok = textparser_get_token_child(cchild);
                for (size_t k = 0; k < en; k++, etok = textparser_get_token_next(etok)) {
                    outTokens.push_back(convertToken(etok));
                }
                ok = true;
                break;
            }
        }
    }
    textparser_close(h);
    return ok;
}

} // namespace

// ---------------------------------------------------------------------------
// cf_precisionevaluate
// ---------------------------------------------------------------------------

cfvariant *cf_precisionevaluate(const cfvariant *expr,
                                void *cgi, void *server, void *cookie, void *application,
                                void *session, void *url, void *form, void *variables)
{
    if (!expr) throw webstrada::exception("PrecisionEvaluate requires 1 argument");
    std::string input = varToStdString(*expr);

    // CF's _PrecisionEvaluate: a string argument containing a comma is split
    // into a list of expressions, each evaluated in turn; the last wins.
    std::vector<std::string> parts;
    if (input.find(',') != std::string::npos) {
        size_t start = 0, pos;
        while ((pos = input.find(',', start)) != std::string::npos) {
            parts.push_back(input.substr(start, pos - start));
            start = pos + 1;
        }
        parts.push_back(input.substr(start));
    } else {
        parts.push_back(input);
    }

    cfvariant *last = nullptr;
    for (const std::string &rawPart : parts) {
        std::string part = rawPart;
        size_t b = 0;
        while (b < part.size() && (part[b] == ' ' || part[b] == '\t')) b++;
        size_t e = part.size();
        while (e > b && (part[e - 1] == ' ' || part[e - 1] == '\t')) e--;
        part = part.substr(b, e - b);

        if (last) { delete last; last = nullptr; }

        // PrecisionEvalExprClassLoader.evaluate fast paths.
        if (part.empty()) {
            last = new cfvariant(part.c_str());
            continue;
        }
        {
            std::string low = part;
            std::transform(low.begin(), low.end(), low.begin(),
                [](unsigned char c) { return static_cast<char>(tolower(c)); });
            if (low == "true" || low == "false") {
                last = new cfvariant(part.c_str());
                continue;
            }
        }
        // A bare identifier (possibly dotted: form.x) resolves to the
        // variable's value (_autoscalarize).
        {
            bool ident = !part.empty() &&
                (isalpha(static_cast<unsigned char>(part[0])) || part[0] == '_');
            for (char c : part) {
                if (!(isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.')) { ident = false; break; }
            }
            if (ident) {
                cfvariant *val = cfvariant_bare_identifier(
                    static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                    static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                    static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                    static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                    part.c_str());
                last = new cfvariant(*val);
                continue;
            }
        }
        // A bare number literal returns the BigDecimal directly.
        {
            // quick strict number check
            bool number = true;
            {
                size_t i = 0;
                if (i < part.size() && (part[i] == '+' || part[i] == '-')) i++;
                bool sawDigit = false, sawDot = false, inExp = false;
                for (; i < part.size(); i++) {
                    char c = part[i];
                    if (c >= '0' && c <= '9') { sawDigit = true; continue; }
                    if (!inExp && c == '.') { sawDot = true; continue; }
                    if (!inExp && (c == 'e' || c == 'E') && sawDigit) {
                        inExp = true;
                        if (i + 1 < part.size() && (part[i + 1] == '+' || part[i + 1] == '-')) i++;
                        continue;
                    }
                    number = false;
                    break;
                }
                if (!sawDigit) number = false;
            }
            if (number) {
                try {
                    CfBigDecimal bd = parseBigDecimal(part);
                    last = bdResult(bd);
                    continue;
                } catch (const webstrada::exception &) {
                    // not a plain number after all — fall through to the parser
                }
            }
        }

        // Otherwise parse with the engine's expression parser and evaluate the
        // AST with BigDecimal semantics.
        std::string wrappedText;
        std::vector<TextParserTokenItem> tokens;
        if (!tokenizePrecisionExpr(part, wrappedText, tokens)) {
            throw webstrada::exception("Invalid CFML construct found.");
        }
        // CF's precision grammar rejects the symbolic < > <= >= comparisons.
        for (const auto &tok : tokens) {
            if (!isOperatorToken(tok.token_id)) continue;
            std::string op(wrappedText.c_str() + tok.position, tok.len);
            while (!op.empty() && isspace((unsigned char)op.front())) op.erase(op.begin());
            while (!op.empty() && isspace((unsigned char)op.back())) op.pop_back();
            for (auto &ch : op) ch = static_cast<char>(toupper((unsigned char)ch));
            if (op == "<" || op == ">" || op == "<=" || op == ">=") {
                throw webstrada::exception("Invalid CFML construct found.");
            }
        }
        auto ast = parseTokensToAST(tokens, wrappedText.c_str(), true);
        EvalCtx ctx{wrappedText.c_str(), cgi, server, cookie, application, session, url, form, variables};
        PeValue result = evalPrecision(*ast, ctx);
        last = finish(result);
    }
    if (!last) last = new cfvariant("");
    return last;
}

} // namespace cfml

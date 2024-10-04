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

namespace cfml {

// Seed the C rand() generator from the system entropy so fresh processes do not
// produce an identical CreateUUID()/CreateGUID() sequence (the default srand(1)
// seed). The daemon forks workers after calling this once, so the worker loop
// re-seeds too (each worker then gets a distinct seed).
void seed_rand() {
    unsigned seed = static_cast<unsigned>(time(nullptr));
    seed ^= static_cast<unsigned>(getpid()) << 8;
    seed ^= static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::srand(seed);
}

} // namespace cfml

// ---- Misc implemented functions ----

static bool utf8Decode(const char *data, int len, std::vector<uint32_t> &cps) {
    int i = 0;
    while (i < len) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        uint32_t cp;
        int extra;
        if (c < 0x80) {
            cp = c; extra = 0;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F; extra = 1;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F; extra = 2;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07; extra = 3;
        } else {
            return false;
        }
        if (i + extra >= len) return false;
        for (int k = 1; k <= extra; k++) {
            unsigned char cc = static_cast<unsigned char>(data[i + k]);
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        cps.push_back(cp);
        i += extra + 1;
    }
    return true;
}

// Encode a list of codepoints into the requested charset, writing into any
// contiguous byte buffer (`std::vector<std::byte>` / `std::vector<char>`).
template <typename ByteOut>
static void codepointsToBytes(const std::vector<uint32_t> &cps, const webstrada::string &encoding, ByteOut &out) {
    webstrada::string enc = encoding;
    enc.toUpper();
    if (enc.equals("UTF-8") || enc.equals("UTF8")) {
        for (auto cp : cps) {
            if (cp < 0x80) {
                out.push_back(static_cast<typename ByteOut::value_type>(cp));
            } else if (cp < 0x800) {
                out.push_back(static_cast<typename ByteOut::value_type>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<typename ByteOut::value_type>(0x80 | (cp & 0x3F)));
            } else if (cp < 0x10000) {
                out.push_back(static_cast<typename ByteOut::value_type>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<typename ByteOut::value_type>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<typename ByteOut::value_type>(0x80 | (cp & 0x3F)));
            } else {
                out.push_back(static_cast<typename ByteOut::value_type>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<typename ByteOut::value_type>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<typename ByteOut::value_type>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<typename ByteOut::value_type>(0x80 | (cp & 0x3F)));
            }
        }
    } else if (enc.equals("ISO-8859-1") || enc.equals("LATIN1") || enc.equals("ISO8859-1") || enc.equals("LATIN-1")) {
        for (auto cp : cps) {
            out.push_back(static_cast<typename ByteOut::value_type>(cp > 0xFF ? 0x3F : cp));
        }
    } else if (enc.equals("US-ASCII") || enc.equals("ASCII") || enc.equals("ISO646-US")) {
        // US-ASCII only maps 0x00-0x7F; unmappable characters become '?' (0x3F).
        for (auto cp : cps) {
            out.push_back(static_cast<typename ByteOut::value_type>(cp > 0x7F ? 0x3F : cp));
        }
    } else if (enc.equals("UTF-16") || enc.equals("UTF-16BE") || enc.equals("UTF-16LE") || enc.equals("UTF16")) {
        bool be = !enc.equals("UTF-16LE");
        bool withBom = enc.equals("UTF-16");
        if (withBom) {
            if (be) { out.push_back(static_cast<typename ByteOut::value_type>(0xFE)); out.push_back(static_cast<typename ByteOut::value_type>(0xFF)); }
            else { out.push_back(static_cast<typename ByteOut::value_type>(0xFF)); out.push_back(static_cast<typename ByteOut::value_type>(0xFE)); }
        }
        for (auto cp : cps) {
            if (cp < 0x10000) {
                if (be) { out.push_back(static_cast<typename ByteOut::value_type>((cp >> 8) & 0xFF)); out.push_back(static_cast<typename ByteOut::value_type>(cp & 0xFF)); }
                else { out.push_back(static_cast<typename ByteOut::value_type>(cp & 0xFF)); out.push_back(static_cast<typename ByteOut::value_type>((cp >> 8) & 0xFF)); }
            } else {
                uint32_t v = cp - 0x10000;
                uint32_t hi = 0xD800 + (v >> 10);
                uint32_t lo = 0xDC00 + (v & 0x3FF);
                if (be) { out.push_back(static_cast<typename ByteOut::value_type>((hi >> 8) & 0xFF)); out.push_back(static_cast<typename ByteOut::value_type>(hi & 0xFF)); out.push_back(static_cast<typename ByteOut::value_type>((lo >> 8) & 0xFF)); out.push_back(static_cast<typename ByteOut::value_type>(lo & 0xFF)); }
                else { out.push_back(static_cast<typename ByteOut::value_type>(hi & 0xFF)); out.push_back(static_cast<typename ByteOut::value_type>((hi >> 8) & 0xFF)); out.push_back(static_cast<typename ByteOut::value_type>(lo & 0xFF)); out.push_back(static_cast<typename ByteOut::value_type>((lo >> 8) & 0xFF)); }
            }
        }
    } else {
        throw webstrada::exception("The character encoding '" + encoding + "' is not supported");
    }
}

// Decode a CFML string's bytes to codepoints: as UTF-8 when possible, otherwise
// as Latin-1 bytes (the shared front half of both stringToBytes overloads).
static std::vector<uint32_t> utf8ToCodepoints(const webstrada::string &s) {
    std::vector<uint32_t> cps;
    const char *data = s.constData();
    int len = s.length();
    if (!utf8Decode(data, len, cps)) {
        cps.clear();
        for (int i = 0; i < len; i++) {
            cps.push_back(static_cast<unsigned char>(data[i]));
        }
    }
    return cps;
}

// Convert a CFML string to raw bytes using the requested character encoding
// name. Input bytes are decoded as UTF-8 when possible; otherwise they are
// treated as Latin-1 bytes.
void cfml::stringToBytes(const webstrada::string &s, const webstrada::string &encoding, std::vector<std::byte> &out) {
    webstrada::string enc = encoding;
    if (enc.isEmpty()) enc = "UTF-8";
    codepointsToBytes(utf8ToCodepoints(s), enc, out);
}

// Same conversion writing into a plain char buffer (the output-send path uses
// this to avoid an intermediate std::vector<std::byte>).
void cfml::stringToBytes(const webstrada::string &s, const webstrada::string &encoding, std::vector<char> &out) {
    webstrada::string enc = encoding;
    if (enc.isEmpty()) enc = "UTF-8";
    codepointsToBytes(utf8ToCodepoints(s), enc, out);
}


// Normalize and validate a Java-style charset name (case-insensitive, accepts
// '_' variants like UTF_16 / ISO8859_1). Returns a canonical name understood
// by codepointsToBytes/bytesToText, or throws the CF error for unknown names.
webstrada::string cfml::normalizeCharsetName(const webstrada::string &encoding) {
    webstrada::string enc = encoding;
    enc.toUpper();
    webstrada::string n;
    for (int i = 0; i < enc.length(); i++) {
        char c = enc.at(i);
        n.append(c == '_' ? '-' : c);
    }
    if (n.equals("UTF8")) return "UTF-8";
    if (n.equals("UTF16")) return "UTF-16";
    if (n.equals("LATIN1") || n.equals("LATIN-1") || n.equals("ISO8859-1") || n.equals("8859-1")) return "ISO-8859-1";
    if (n.equals("ASCII") || n.equals("ISO646-US")) return "US-ASCII";
    if (n.equals("UTF-8") || n.equals("ISO-8859-1") || n.equals("US-ASCII") || n.equals("UTF-16") ||
        n.equals("UTF-16BE") || n.equals("UTF-16LE")) {
        return n;
    }
    // CF wraps an empty encoding name in quotes: "Unsupported encoding format ''."
    if (encoding.isEmpty()) {
        throw webstrada::exception("Unsupported encoding format ''.");
    }
    throw webstrada::exception("Unsupported encoding format " + encoding + ".");
}

// ==========================================
// URL encode/decode helpers
// ==========================================

static int urlHexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Percent-decode a URL string into raw bytes. '+' becomes a space and '%XX'
// (case-insensitive hex) becomes one byte. In strict mode a '%' that is not
// followed by two hex digits throws (DecodeFromURL / ESAPI behavior); in
// lenient mode it is passed through literally (URLDecode behavior).
void cfml::urlDecodeString(const webstrada::string &s, std::vector<std::byte> &out, bool strict) {
    const char *data = s.constData();
    int len = s.length();
    for (int i = 0; i < len; i++) {
        char c = data[i];
        if (c == '+') {
            out.push_back(std::byte(' '));
        } else if (c == '%') {
            if (i + 2 < len) {
                int hi = urlHexDigit(data[i + 1]);
                int lo = urlHexDigit(data[i + 2]);
                if (hi >= 0 && lo >= 0) {
                    out.push_back(std::byte((hi << 4) | lo));
                    i += 2;
                    continue;
                }
            }
            if (strict) {
                throw webstrada::exception("DecodeFromURL: Invalid URL-encoded string");
            }
            out.push_back(std::byte('%'));
        } else {
            out.push_back(std::byte(static_cast<unsigned char>(c)));
        }
    }
}

// Decode raw bytes using the requested character encoding. Invalid UTF-8
// sequences produce U+FFFD, matching the JDK decoder replacement behavior.
// The returned string stores codepoints as UTF-8 (the runtime's internal
// string encoding).
webstrada::string cfml::bytesToText(const std::vector<std::byte> &bytes, const webstrada::string &encoding) {
    webstrada::string enc = encoding;
    if (enc.isEmpty()) enc = "UTF-8";
    enc.toUpper();
    const char *data = reinterpret_cast<const char*>(bytes.data());
    int len = static_cast<int>(bytes.size());
    std::vector<uint32_t> cps;
    if (enc.equals("UTF-8") || enc.equals("UTF8")) {
        int i = 0;
        while (i < len) {
            unsigned char c = static_cast<unsigned char>(data[i]);
            uint32_t cp;
            int extra;
            if (c < 0x80) {
                cp = c; extra = 0;
            } else if ((c & 0xE0) == 0xC0) {
                cp = c & 0x1F; extra = 1;
            } else if ((c & 0xF0) == 0xE0) {
                cp = c & 0x0F; extra = 2;
            } else if ((c & 0xF8) == 0xF0) {
                cp = c & 0x07; extra = 3;
            } else {
                cps.push_back(0xFFFD);
                i++;
                continue;
            }
            if (i + extra >= len) {
                cps.push_back(0xFFFD);
                i++;
                continue;
            }
            bool ok = true;
            for (int k = 1; k <= extra; k++) {
                unsigned char cc = static_cast<unsigned char>(data[i + k]);
                if ((cc & 0xC0) != 0x80) { ok = false; break; }
                cp = (cp << 6) | (cc & 0x3F);
            }
            if (!ok) {
                cps.push_back(0xFFFD);
                i++;
                continue;
            }
            cps.push_back(cp);
            i += extra + 1;
        }
    } else if (enc.equals("ISO-8859-1") || enc.equals("LATIN1") || enc.equals("ISO8859-1") || enc.equals("LATIN-1")) {
        for (int i = 0; i < len; i++) {
            cps.push_back(static_cast<unsigned char>(data[i]));
        }
    } else if (enc.equals("US-ASCII") || enc.equals("ASCII") || enc.equals("ISO646-US")) {
        // US-ASCII only maps 0x00-0x7F; unmappable bytes decode to U+FFFD.
        for (int i = 0; i < len; i++) {
            unsigned char b = static_cast<unsigned char>(data[i]);
            cps.push_back(b < 0x80 ? b : 0xFFFD);
        }
    } else if (enc.equals("UTF-16") || enc.equals("UTF-16BE") || enc.equals("UTF-16LE") || enc.equals("UTF16")) {
        bool be = !enc.equals("UTF-16LE");
        int start = 0;
        if (enc.equals("UTF-16") && len >= 2) {
            unsigned char b0 = static_cast<unsigned char>(data[0]);
            unsigned char b1 = static_cast<unsigned char>(data[1]);
            if (b0 == 0xFE && b1 == 0xFF) { be = true; start = 2; }
            else if (b0 == 0xFF && b1 == 0xFE) { be = false; start = 2; }
        }
        for (int i = start; i + 1 < len; i += 2) {
            unsigned char hi = static_cast<unsigned char>(data[i]);
            unsigned char lo = static_cast<unsigned char>(data[i + 1]);
            uint32_t u = be ? ((hi << 8) | lo) : ((lo << 8) | hi);
            if (u >= 0xD800 && u <= 0xDBFF && i + 3 < len) {
                unsigned char hi2 = static_cast<unsigned char>(data[i + 2]);
                unsigned char lo2 = static_cast<unsigned char>(data[i + 3]);
                uint32_t u2 = be ? ((hi2 << 8) | lo2) : ((lo2 << 8) | hi2);
                if (u2 >= 0xDC00 && u2 <= 0xDFFF) {
                    cps.push_back(0x10000 + ((u - 0xD800) << 10) + (u2 - 0xDC00));
                    i += 2;
                    continue;
                }
            }
            cps.push_back(u);
        }
    } else {
        throw webstrada::exception("The character encoding '" + encoding + "' is not supported");
    }
    // Store codepoints as UTF-8 bytes (internal string encoding).
    webstrada::string out;
    for (auto cp : cps) {
        if (cp < 0x80) {
            out.append(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.append(static_cast<char>(0xC0 | (cp >> 6)));
            out.append(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.append(static_cast<char>(0xE0 | (cp >> 12)));
            out.append(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.append(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.append(static_cast<char>(0xF0 | (cp >> 18)));
            out.append(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.append(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.append(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}


void cfml::request_set_body(const char *data, size_t len) {
    if (!data || len == 0) {
        g_requestBody.clear();
        return;
    }
    g_requestBody.assign(data, len);
}

static bool canParseAsDouble(const string &s) {
    if (s.isEmpty()) return false;
    char *endptr = nullptr;
    const char *str = s.constData();
    while (*str && std::isspace(static_cast<unsigned char>(*str))) str++;
    if (!*str) return false;
    std::strtod(str, &endptr);
    while (endptr && *endptr && std::isspace(static_cast<unsigned char>(*endptr))) endptr++;
    return endptr && (*endptr == '\0');
}

cfvariant *cfml::cf_applicationstop() {
    auto &sc = scope_context();
    if (sc.store && sc.applicationEnabled && !sc.appName.empty()) {
        // Remove the application and all of its sessions from the store.
        sc.store->removeApplication(sc.appName);
    }
    // The current request keeps its application/session data (verified on CF:
    // StructKeyExists(application, ...) stays YES after ApplicationStop), but
    // the app is stopped: scope_end must NOT write the scopes back (that would
    // recreate the rows), so disable the persistence context here.
    sc.applicationEnabled = false;
    sc.sessionEnabled = false;
    sc.appName.clear();
    sc.sessionId.clear();

    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

cfvariant *cfml::cf_bitand(const cfvariant *n1, const cfvariant *n2) {
    if (!n1 || !n2) throw webstrada::exception("BitAnd requires exactly 2 arguments");
    int val = getIntValue(*n1) & getIntValue(*n2);
    auto *ret = new cfvariant(val);
    return ret;
}

cfvariant *cfml::cf_bitmaskclear(const cfvariant *num, const cfvariant *start, const cfvariant *len) {
    if (!num || !start || !len) throw webstrada::exception("BitMaskClear requires exactly 3 arguments");
    int val = getIntValue(*num);
    int s = getIntValue(*start);
    int l = getIntValue(*len);
    if (s < 0 || s > 31 || l < 0 || l > 31) throw webstrada::exception("BitMaskClear: Start and length must be between 0 and 31");
    unsigned int mask = 0;
    if (l > 0) {
        if (l >= 32) mask = 0xFFFFFFFFu;
        else mask = ((1u << l) - 1u) << s;
    }
    val = val & (~static_cast<int>(mask));
    auto *ret = new cfvariant(val);
    return ret;
}

cfvariant *cfml::cf_bitmaskread(const cfvariant *num, const cfvariant *start, const cfvariant *len) {
    if (!num || !start || !len) throw webstrada::exception("BitMaskRead requires exactly 3 arguments");
    int val = getIntValue(*num);
    int s = getIntValue(*start);
    int l = getIntValue(*len);
    if (s < 0 || s > 31 || l < 0 || l > 31) throw webstrada::exception("BitMaskRead: Start and length must be between 0 and 31");
    unsigned int res = 0;
    if (l > 0) {
        unsigned int uval = static_cast<unsigned int>(val);
        res = (uval >> s);
        if (l < 32) res &= ((1u << l) - 1u);
    }
    auto *ret = new cfvariant(static_cast<int>(res));
    return ret;
}

cfvariant *cfml::cf_bitmaskset(const cfvariant *num, const cfvariant *mask, const cfvariant *start, const cfvariant *len) {
    if (!num || !mask || !start || !len) throw webstrada::exception("BitMaskSet requires exactly 4 arguments");
    int val = getIntValue(*num);
    int m = getIntValue(*mask);
    int s = getIntValue(*start);
    int l = getIntValue(*len);
    if (s < 0 || s > 31 || l < 0 || l > 31) throw webstrada::exception("BitMaskSet: Start and length must be between 0 and 31");
    unsigned int clearMask = 0;
    if (l > 0) {
        if (l >= 32) clearMask = 0xFFFFFFFFu;
        else clearMask = ((1u << l) - 1u) << s;
    }
    val = val & (~static_cast<int>(clearMask));
    unsigned int mShifted = (static_cast<unsigned int>(m) << s) & clearMask;
    val = val | static_cast<int>(mShifted);
    auto *ret = new cfvariant(val);
    return ret;
}

cfvariant *cfml::cf_bitnot(const cfvariant *num) {
    if (!num) throw webstrada::exception("BitNot requires exactly 1 argument");
    int val = ~getIntValue(*num);
    auto *ret = new cfvariant(val);
    return ret;
}

cfvariant *cfml::cf_bitor(const cfvariant *n1, const cfvariant *n2) {
    if (!n1 || !n2) throw webstrada::exception("BitOr requires exactly 2 arguments");
    int val = getIntValue(*n1) | getIntValue(*n2);
    auto *ret = new cfvariant(val);
    return ret;
}

cfvariant *cfml::cf_bitshln(const cfvariant *num, const cfvariant *count) {
    if (!num || !count) throw webstrada::exception("BitSHLN requires exactly 2 arguments");
    int val = getIntValue(*num);
    int cnt = getIntValue(*count);
    if (cnt < 0 || cnt > 31) throw webstrada::exception("BitSHLN: Count must be between 0 and 31");
    val = val << cnt;
    auto *ret = new cfvariant(val);
    return ret;
}

cfvariant *cfml::cf_bitshrn(const cfvariant *num, const cfvariant *count) {
    if (!num || !count) throw webstrada::exception("BitSHRN requires exactly 2 arguments");
    int val = getIntValue(*num);
    int cnt = getIntValue(*count);
    if (cnt < 0 || cnt > 31) throw webstrada::exception("BitSHRN: Count must be between 0 and 31");
    unsigned int uval = static_cast<unsigned int>(val);
    uval = uval >> cnt;
    auto *ret = new cfvariant(static_cast<int>(uval));
    return ret;
}

cfvariant *cfml::cf_bitxor(const cfvariant *n1, const cfvariant *n2) {
    if (!n1 || !n2) throw webstrada::exception("BitXor requires exactly 2 arguments");
    int val = getIntValue(*n1) ^ getIntValue(*n2);
    auto *ret = new cfvariant(val);
    return ret;
}

cfvariant *cfml::cf_booleanformat(const cfvariant *val) {
    if (!val) throw webstrada::exception("BooleanFormat requires exactly 1 argument");
    bool b = isTruthy(*val);
    auto *ret = new cfvariant(b ? "true" : "false");
    return ret;
}

cfvariant *cfml::cf_charsetdecode(const cfvariant *str, const cfvariant *encoding) {
    if (!str || !encoding) throw webstrada::exception("CharsetDecode requires exactly 2 arguments");
    webstrada::string enc = normalizeCharsetName(variantToString(*encoding));
    std::vector<std::byte> bytes;
    stringToBytes(variantToString(*str), enc, bytes);
    auto *ret = new cfvariant(cfvariant::Binary);
    *ret->m_binary = std::move(bytes);
    return ret;
}

cfvariant *cfml::cf_charsetencode(const cfvariant *binaryData, const cfvariant *encoding) {
    if (!binaryData || !encoding) throw webstrada::exception("CharsetEncode requires exactly 2 arguments");
    if (binaryData->m_type != cfvariant::Binary || !binaryData->m_binary) {
        throw webstrada::exception("Parameter 1 of the CharsetEncode function, which is now " + variantToString(*binaryData) + ", must be a valid binary object.");
    }
    webstrada::string enc = normalizeCharsetName(variantToString(*encoding));
    webstrada::string out = bytesToText(*binaryData->m_binary, enc);
    auto *ret = new cfvariant(out);
    return ret;
}

cfvariant *cfml::cf_decodefromurl(const cfvariant *str) {
    if (!str) throw webstrada::exception("DecodeFromURL requires exactly 1 argument");
    webstrada::string input = variantToString(*str);
    std::vector<std::byte> bytes;
    urlDecodeString(input, bytes, true);
    webstrada::string out = bytesToText(bytes, "UTF-8");
    auto *ret = new cfvariant(out);
    return ret;
}

cfvariant *cfml::cf_deserializexml(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2) {
    return cf_xmlparse(arg0, arg1, arg2);
}

cfvariant *cfml::cf_evaluate(string &out, const cfvariant **args, int arg_count,
    void *cgi, void *server, void *cookie, void *application,
    void *session, void *url, void *form, void *variables) {
    if (arg_count < 1) {
        throw webstrada::exception("Parameter validation error for the EVALUATE function: The function takes 1 or more parameters.");
    }
    cfvariant result;
    for (int i = 0; i < arg_count; i++) {
        if (!args[i]) throw webstrada::exception("Parameter validation error for the EVALUATE function: The function takes 1 or more parameters.");
        string exprStr = const_cast<cfvariant*>(args[i])->toString();
        result = evaluateExpr(out, exprStr, cgi, server, cookie, application, session, url, form, variables, true);
    }
    auto *ret = new cfvariant(result);
    return ret;
}

cfvariant *cfml::cf_findoneof(const cfvariant *set, const cfvariant *str, const cfvariant *start) {
    if (!set || !str) throw webstrada::exception("FindOneOf requires at least 2 arguments");
    string setStr = variantToString(*set);
    string strStr = variantToString(*str);
    int st = start ? getIntValue(*start) : 1;
    if (st < 1 || st > (int)strStr.length()) {
        auto *ret = new cfvariant(0);
        return ret;
    }
    int foundPos = 0;
    for (size_t i = st - 1; i < (size_t)strStr.length(); i++) {
        char c = strStr.at(static_cast<int>(i));
        if (setStr.indexOf(c) != -1) {
            foundPos = static_cast<int>(i + 1);
            break;
        }
    }
    auto *ret = new cfvariant(foundPos);
    return ret;
}

cfvariant *cfml::cf_firstdayofmonth(const cfvariant *date) {
    if (!date) throw webstrada::exception("FirstDayOfMonth requires exactly 1 argument");
    double days = getDaysOrThrow(date, "FirstDayOfMonth");
    struct tm tm = daysToTm(days);
    struct tm first = tm;
    first.tm_mday = 1;
    first.tm_hour = 0; first.tm_min = 0; first.tm_sec = 0;
    double first_days = tmToDays(first);
    struct tm first_normal = daysToTm(first_days);
    auto *ret = new cfvariant(first_normal.tm_yday + 1);
    return ret;
}

cfvariant *cfml::cf_fix(const cfvariant *num) {
    if (!num) throw webstrada::exception("Fix requires exactly 1 argument");
    double d = getDoubleValue(*num);
    double fixedVal = (d >= 0) ? std::floor(d) : std::ceil(d);
    cfvariant res(cfvariant::Float);
    res.m_double = fixedVal;
    auto *ret = new cfvariant(res);
    return ret;
}

cfvariant *cfml::cf_formatbasen(const cfvariant *num, const cfvariant *radix) {
    if (!num || !radix) throw webstrada::exception("FormatBaseN requires exactly 2 arguments");
    long long nll = getLongIntValue(*num);
    int r = getIntValue(*radix);
    if (r < 2 || r > 36) throw webstrada::exception("FormatBaseN: Radix must be between 2 and 36");
    if (nll < -2147483648LL || nll > 4294967295LL) {
        throw webstrada::exception("FormatBaseN: Number must be between -2147483648 and 4294967295");
    }
    int32_t n = static_cast<int32_t>(static_cast<uint32_t>(nll));
    std::string s;
    if (r == 2) {
        uint32_t un = static_cast<uint32_t>(n);
        if (n < 0) {
            for (int i = 31; i >= 0; i--) s += ((un >> i) & 1) ? '1' : '0';
        } else if (un == 0) {
            s = "0";
        } else {
            while (un > 0) { s += (un & 1) ? '1' : '0'; un >>= 1; }
            std::reverse(s.begin(), s.end());
        }
    } else if (r == 16) {
        const char digits[] = "0123456789abcdef";
        uint32_t un = static_cast<uint32_t>(n);
        if (n < 0) {
            for (int i = 7; i >= 0; i--) s += digits[(un >> (i * 4)) & 0xf];
        } else if (un == 0) {
            s = "0";
        } else {
            while (un > 0) { s += digits[un & 0xf]; un >>= 4; }
            std::reverse(s.begin(), s.end());
        }
    } else {
        if (n == 0) {
            s = "0";
        } else {
            long long un = (n < 0) ? -(long long)n : n;
            const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
            while (un > 0) { s += digits[un % r]; un /= r; }
            if (n < 0) s += '-';
            std::reverse(s.begin(), s.end());
        }
    }
    auto *ret = new cfvariant(s.c_str());
    return ret;
}

cfvariant *cfml::cf_getapplicationmetadata() {
    auto &sc = scope_context();
    cfvariant ret(cfvariant::Struct);
    // CF returns an EMPTY struct when no application is active (no
    // <cfapplication>/Application.cfc ran) — AppHelper.getApplicationMetaData
    // returns null and CFPage.GetApplicationMetadata falls back to a fresh
    // empty Struct (was BUGS.md "GetApplicationMetadata returns a subset").
    if (!sc.applicationEnabled) {
        auto *out = new cfvariant(ret);
        return out;
    }
    auto cfBool = [](bool b) {
        cfvariant v(cfvariant::Boolean);
        v.m_bool = b;
        v.m_boolLiteral = false; // computed -> renders YES/NO like CF
        return v;
    };

    if (sc.applicationEnabled && !sc.appName.empty()) {
        ret.structSet("NAME", cfvariant(sc.appName.c_str()));
    } else {
        ret.structSet("NAME", cfvariant(""));
    }
    // ColdFusion reports the timeouts as whole seconds (e.g. APPLICATIONTIMEOUT
    // = 93784 for CreateTimeSpan(1,2,3,4), SESSIONTIMEOUT = 1800 for 30 min).
    ret.structSet("APPLICATIONTIMEOUT", cfvariant((int)sc.appTimeoutSeconds));
    ret.structSet("SESSIONMANAGEMENT", cfBool(sc.sessionManagement));
    ret.structSet("SESSIONTIMEOUT", cfvariant((int)sc.sessionTimeoutSeconds));
    ret.structSet("CLIENTMANAGEMENT", cfBool(false));
    ret.structSet("SETCLIENTCOOKIES", cfBool(true));
    ret.structSet("SETDOMAINCOOKIES", cfBool(false));
    ret.structSet("LOGINSTORAGE", cfvariant("cookie"));

    auto *out = new cfvariant(ret);
    return out;
}

cfvariant *cfml::cf_gethttprequestdata(void *cgi, const cfvariant *includeBody) {
    // Returns a struct with the HTTP request headers, method, protocol and
    // body, like CF's GetHttpRequestData(). The headers are read from the CGI
    // scope's HTTP_* variables (each is lowercased and the HTTP_ prefix
    // stripped), the method/protocol from REQUEST_METHOD / SERVER_PROTOCOL,
    // and the content from the request body captured by the worker. The
    // `includeBody` argument (default true) controls whether the `content` key
    // is present at all — verified on CF 2025: GetHttpRequestData() includes
    // content, GetHttpRequestData(false) omits the key entirely.
    cfvariant result(cfvariant::Struct);

    // headers
    cfvariant headers(cfvariant::Struct);
    if (cgi) {
        const cfvariant *cgiScope = static_cast<const cfvariant*>(cgi);
        if (cgiScope->m_type == cfvariant::Struct && cgiScope->m_struct) {
            for (auto &kv : *cgiScope->m_struct) {
                std::string key(kv.first.constData(), kv.first.length());
                if (key.rfind("HTTP_", 0) == 0 && key.length() > 5) {
                    std::string hname = key.substr(5);
                    for (auto &c : hname) c = static_cast<char>(tolower(c));
                    cfvariant val = kv.second;
                    headers.set(hname.c_str()) = val;
                }
            }
        }
    }
    result.set("headers") = headers;

    // method / protocol
    auto cgiGet = [&](const char *key) -> std::string {
        if (!cgi) return "";
        const cfvariant *cgiScope = static_cast<const cfvariant*>(cgi);
        if (cgiScope->m_type != cfvariant::Struct || !cgiScope->m_struct) return "";
        webstrada::string k(key);
        auto it = cgiScope->m_struct->find(k);
        if (it == cgiScope->m_struct->end()) return "";
        return safe_to_std_string(it->second);
    };
    // CF inserts the keys in the order headers, protocol, method, content
    // (verified: StructKeyList(GetHttpRequestData()) == "headers,protocol,method,content").
    result.set("protocol") = cfvariant(cgiGet("SERVER_PROTOCOL").c_str());
    result.set("method") = cfvariant(cgiGet("REQUEST_METHOD").c_str());

    // content (only when includeBody is true; the default is true)
    bool wantBody = true;
    if (includeBody && includeBody->m_type != cfvariant::Null) {
        wantBody = isTruthy(*includeBody);
    }
    if (wantBody) {
        result.set("content") = cfvariant(g_requestBody.c_str());
    }

    auto *ret = new cfvariant(result);
    return ret;
}

cfvariant *cfml::cf_inputbasen(const cfvariant *str, const cfvariant *radix) {
    if (!str || !radix) throw webstrada::exception("InputBaseN requires exactly 2 arguments");
    string s = variantToString(*str);
    int r = getIntValue(*radix);
    if (r < 2 || r > 36) throw webstrada::exception("InputBaseN: Radix must be between 2 and 36");
    char *endptr = nullptr;
    long long val = std::strtoll(s.constData(), &endptr, r);
    auto *ret = new cfvariant(static_cast<int>(val));
    return ret;
}

cfvariant *cfml::cf_insert(const cfvariant *sub, const cfvariant *str, const cfvariant *pos) {
    if (!sub || !str || !pos) throw webstrada::exception("Insert requires exactly 3 arguments");
    string subStr = variantToString(*sub);
    string strStr = variantToString(*str);
    int position = getIntValue(*pos);
    if (position < 0 || position > strStr.length()) {
        throw webstrada::exception("Insert: Position out of bounds");
    }
    string result = strStr.left(position);
    result.append(subStr);
    if (position < strStr.length()) {
        result.append(strStr.mid(position, strStr.length() - position));
    }
    auto *ret = new cfvariant(result);
    return ret;
}

cfvariant *cfml::cf_isdefined(const cfvariant *a1, void *cgi, void *server, void *cookie,
                              void *application, void *session, void *url, void *form,
                              void *variables) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    if (!a1) return ret;

    string name = const_cast<cfvariant*>(a1)->toString();
    string upName = name;
    upName.toUpper();

    // ColdFusion quirk (verified on CF 2025): IsDefined on any `cgi.<member>`
    // reference returns YES even for keys that do not exist (cgi.NOPE,
    // cgi.<empty>, cgi.a.b all report YES). A bare `cgi` resolves normally
    // below (the CGI scope exists, so it is YES too).
    if (upName.startWith("CGI.")) {
        ret->m_bool = true;
        return ret;
    }

    if (name.isEmpty()) return ret;

    // The `local` scope only exists inside a UDF (verified on CF 2025: inside a
    // function IsDefined("local") and IsDefined("local.lv") are YES, at page
    // level they are NO). Map it to the current function's local scope.
    string firstName = upName;
    int firstDot = upName.indexOf('.');
    if (firstDot >= 0) firstName = upName.left(firstDot);
    if (firstName.equals("LOCAL")) {
        if (g_udfCtx.empty()) return ret;
        cfvariant *ls = g_udfCtx.back().localScope;
        if (!ls || ls->m_type != cfvariant::Struct || ls->m_disabled) return ret;
        cfvariant *cur = ls;
        bool ok = true;
        if (firstDot >= 0) {
            std::vector<string> parts = upName.mid(firstDot + 1, upName.length() - firstDot - 1).split('.');
            for (size_t i = 0; i < parts.size(); i++) {
                if (cur && cur->m_type == cfvariant::Query && cur->m_query) {
                    cur = resolveQueryMember(cur, parts[i].constData());
                    if (!cur) { ok = false; break; }
                    continue;
                }
                if (!cur || (cur->m_type != cfvariant::Struct && cur->m_type != cfvariant::Xml) || cur->m_disabled) {
                    ok = false;
                    break;
                }
                auto it = cur->m_struct->find(parts[i]);
                if (it == cur->m_struct->end()) { ok = false; break; }
                cur = &it->second;
            }
        }
        if (ok && cur) {
            if (cur->m_type == cfvariant::Null || cur->m_type == cfvariant::NotSet) return ret;
            ret->m_bool = true;
        }
        return ret;
    }

    cfvariant *v = lookupVarWritable(name.constData(), cgi, server, cookie, application, session, url, form, variables);
    if (!v) return ret;
    // A null/undefined value is not "defined" (verified on CF 2025:
    // IsDefined("nullvar") and IsDefined("s.n") where the key holds null
    // both return NO). A disabled scope struct (the daemon disables the
    // APPLICATION/SESSION scopes until <cfapplication> enables them) is not
    // defined either: CF returns NO for IsDefined("session") when session
    // management is off.
    if (v->m_type == cfvariant::Null || v->m_type == cfvariant::NotSet) return ret;
    if (v->m_disabled) return ret;
    ret->m_bool = true;
    return ret;
}

cfvariant *cfml::cf_isxml(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("IsXML requires exactly 1 argument");
    bool res = false;
    if (arg->m_type == cfvariant::Xml) {
        res = true;
    } else if (arg->m_type == cfvariant::String) {
        std::string xmlStr = safe_to_std_string(arg->m_str);
        if (!xmlStr.empty()) {
            xmlDocPtr doc = xmlReadMemory(xmlStr.c_str(), xmlStr.length(), "noname.xml", nullptr, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
            if (doc) {
                res = true;
                xmlFreeDoc(doc);
            }
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = res;
    return ret;
}

cfvariant *cfml::cf_isxmlattribute(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("IsXmlAttribute requires exactly 1 argument");
    bool res = false;
    if (arg->m_type == cfvariant::Xml) {
        if (arg->m_struct->contains("XMLTYPE") && arg->m_struct->at("XMLTYPE").toString().equals("ATTRIBUTE")) {
            res = true;
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = res;
    return ret;
}

cfvariant *cfml::cf_isxmldoc(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("IsXmlDoc requires exactly 1 argument");
    bool res = false;
    if (arg->m_type == cfvariant::Xml) {
        if (arg->m_struct->contains("XMLTYPE") && arg->m_struct->at("XMLTYPE").toString().equals("DOCUMENT")) {
            res = true;
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = res;
    return ret;
}

cfvariant *cfml::cf_isxmlelem(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("IsXmlElem requires exactly 1 argument");
    bool res = false;
    if (arg->m_type == cfvariant::Xml) {
        if (arg->m_struct->contains("XMLTYPE") && arg->m_struct->at("XMLTYPE").toString().equals("ELEMENT")) {
            res = true;
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = res;
    return ret;
}

cfvariant *cfml::cf_isxmlnode(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("IsXmlNode requires exactly 1 argument");
    bool res = (arg->m_type == cfvariant::Xml);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = res;
    return ret;
}

cfvariant *cfml::cf_isxmlroot(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("IsXmlRoot requires exactly 1 argument");
    bool res = false;
    if (arg->m_type == cfvariant::Xml) {
        if (arg->m_struct->contains("XMLISROOT")) {
            res = isTruthy(arg->m_struct->at("XMLISROOT"));
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = res;
    return ret;
}

cfvariant *cfml::cf_serializexml(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("SerializeXML requires exactly 1 argument");
    std::string s = serialize_xml_node(*arg);
    auto *ret = new cfvariant(s.c_str());
    return ret;
}

cfvariant *cfml::cf_sessiongetmetadata() {
    auto &sc = scope_context();
    cfvariant ret(cfvariant::Struct);
    if (sc.sessionEnabled && sc.sessionStartTime > 0) {
        // CF returns {STARTTIME: <creation date>} for the current session.
        cfvariant startDate(cfvariant::DateTime);
        // CF dates are days since 1899-12-30; sessionStartTime is a unix epoch.
        startDate.m_double = (double)sc.sessionStartTime / 86400.0 + 25569.0;
        ret.structSet("STARTTIME", startDate);
    } else {
        ret.structSet("STARTTIME", cfvariant(""));
    }
    auto *out = new cfvariant(ret);
    return out;
}

cfvariant *cfml::cf_sessioninvalidate() {
    auto &sc = scope_context();
    if (sc.store && sc.sessionEnabled && !sc.sessionId.empty()) {
        sc.store->removeSession(sc.appName, sc.sessionId);
    }
    // Clear the request's session data; the (now-invalid) cookies stay in the
    // response and the next request mints a fresh session (matching CF).
    if (sc.session) {
        sc.session->set_type(cfvariant::NotSet); // set_type(Struct) is a no-op
        sc.session->set_type(cfvariant::Struct);
    }
    sc.sessionEnabled = false;
    sc.sessionId.clear();
    sc.sessionStartTime = 0;

    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

cfvariant *cfml::cf_sessionrotate() {
    auto &sc = scope_context();
    if (!sc.store || !sc.session) {
        auto *ret = new cfvariant(cfvariant::Null);
        return ret;
    }
    if (!sc.sessionEnabled || sc.sessionId.empty()) {
        throw webstrada::exception("SessionRotate requires an active session");
    }

    // Preserve the current session data, then mint a fresh session id pair
    // and persist the data under the new id (the old id row is removed).
    std::string oldId = sc.sessionId;
    std::string saved = scope_json_serialize(*sc.session);
    int64_t savedStart = sc.sessionStartTime;

    sc.store->removeSession(sc.appName, oldId);

    int64_t newCfid = 1;
    sc.store->nextCfid(newCfid);
    string cfid = std::to_string(newCfid).c_str();
    string token = makeCfToken();
    sc.sessionId = std::string(cfid.constData()) + ":" + token.constData();

    sc.session->set_type(cfvariant::Struct);
    scope_json_deserialize(saved, *sc.session);
    sc.session->m_disabled = false;

    int64_t now = nowSeconds();
    int64_t expiresAt = (sc.sessionTimeoutSeconds > 0)
        ? now + static_cast<int64_t>(sc.sessionTimeoutSeconds) : 0;
    sc.store->storeSession(sc.appName, sc.sessionId, scope_json_serialize(*sc.session), expiresAt, now, savedStart);

    setSessionCookies(cfid, token);
    sc.sessionNewlyCreated = true;

    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

cfvariant *cfml::cf_setday(const cfvariant *date, const cfvariant *day) {
    if (!date || !day) throw webstrada::exception("SetDay requires exactly 2 arguments");
    double days = getDaysOrThrow(date, "SetDay");
    int val = getIntValue(*day);
    struct tm tm = daysToTm(days);
    tm.tm_mday = val;
    normalizeTm(tm);
    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

cfvariant *cfml::cf_sethour(const cfvariant *date, const cfvariant *hour) {
    if (!date || !hour) throw webstrada::exception("SetHour requires exactly 2 arguments");
    double days = getDaysOrThrow(date, "SetHour");
    int val = getIntValue(*hour);
    struct tm tm = daysToTm(days);
    tm.tm_hour = val;
    normalizeTm(tm);
    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

cfvariant *cfml::cf_setminute(const cfvariant *date, const cfvariant *minute) {
    if (!date || !minute) throw webstrada::exception("SetMinute requires exactly 2 arguments");
    double days = getDaysOrThrow(date, "SetMinute");
    int val = getIntValue(*minute);
    struct tm tm = daysToTm(days);
    tm.tm_min = val;
    normalizeTm(tm);
    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

cfvariant *cfml::cf_setmonth(const cfvariant *date, const cfvariant *month) {
    if (!date || !month) throw webstrada::exception("SetMonth requires exactly 2 arguments");
    double days = getDaysOrThrow(date, "SetMonth");
    int val = getIntValue(*month);
    struct tm tm = daysToTm(days);
    tm.tm_mon = val - 1;
    normalizeTm(tm);
    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

cfvariant *cfml::cf_setsecond(const cfvariant *date, const cfvariant *second) {
    if (!date || !second) throw webstrada::exception("SetSecond requires exactly 2 arguments");
    double days = getDaysOrThrow(date, "SetSecond");
    int val = getIntValue(*second);
    struct tm tm = daysToTm(days);
    tm.tm_sec = val;
    normalizeTm(tm);
    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

cfvariant *cfml::cf_setyear(const cfvariant *date, const cfvariant *year) {
    if (!date || !year) throw webstrada::exception("SetYear requires exactly 2 arguments");
    double days = getDaysOrThrow(date, "SetYear");
    int val = getIntValue(*year);
    struct tm tm = daysToTm(days);
    tm.tm_year = val - 1900;
    normalizeTm(tm);
    cfvariant res(cfvariant::DateTime);
    res.m_double = tmToDays(tm);
    auto *ret = new cfvariant(res);
    return ret;
}

cfvariant *cfml::cf_writelog(const cfvariant *text, const cfvariant *type, const cfvariant *application, const cfvariant *file, const cfvariant *log) {
    if (!text) throw webstrada::exception("WriteLog: Missing text argument");

    // Severity: default Information; warning/error/fatal are capitalized; any
    // other value falls back to Information (verified against CF 2021).
    std::string severity = "Information";
    if (type) {
        webstrada::string t = const_cast<cfvariant*>(type)->toString();
        if (t.equals("warning")) severity = "Warning";
        else if (t.equals("error")) severity = "Error";
        else if (t.equals("fatal")) severity = "Fatal";
        else if (t.equals("information")) severity = "Information";
    }

    // Destination: the file attribute wins over log; the default is
    // application.log. CF's LogTag only special-cases log="scheduler"
    // (case-insensitive) into scheduler.log; any other value — "application",
    // "Application", "foo", empty — writes to application.log (verified on CF
    // 2025: log="foo" lands in application.log, not foo.log).
    std::string base = "application";
    if (file) {
        base = safe_to_std_string(*file);
    } else if (log) {
        webstrada::string l = const_cast<cfvariant*>(log)->toString();
        if (!l.isEmpty() && l.compareCaseInsensitive("scheduler") == 0) {
            base = "scheduler";
        }
    }

    // Default log directory is /var/log/WebStrada/ (project convention); an
    // environment override is honored so tests and local runs can redirect it.
    std::string logDir = "/var/log/WebStrada/";
    if (const char *env = getenv("WEBSTRADA_LOG_DIR")) {
        std::string dir(env);
        if (!dir.empty()) {
            if (dir.back() != '/') dir += '/';
            logDir = dir;
        }
    }
    const std::string path = logDir + base + ".log";
    try {
        std::filesystem::create_directories(logDir);
    } catch (...) {
        throw webstrada::exception("WriteLog: Unable to create log directory " + webstrada::string(logDir.c_str()));
    }

    bool isNew = !std::filesystem::exists(path);

    std::string message = safe_to_std_string(*text);

    // Application column: the `application` attribute (default yes) turns on
    // the column; it then carries the active <cfapplication> name uppercased,
    // like CF's LogTag.getApplicationName (ApplicationScope.getName().toUpperCase()).
    // With no active application (or application="no") the column is empty.
    std::string appCol;
    bool logApp = true;
    if (application) logApp = cfmlBoolean(application, true);
    if (logApp) {
        const auto &sc = scope_context();
        if (sc.applicationEnabled && !sc.appName.empty()) {
            appCol = sc.appName;
            for (auto &c : appCol) c = static_cast<char>(toupper((unsigned char)c));
        }
    }

    // Quote fields and double embedded quotes, exactly like CF's CSV log rows.
    auto quote = [](const std::string &s) {
        std::string out = "\"";
        for (char c : s) {
            if (c == '"') out += "\"\"";
            else out += c;
        }
        out += '"';
        return out;
    };

    time_t now = time(nullptr);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char dateBuf[16], timeBuf[16];
    std::snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%02d", tmv.tm_mon + 1, tmv.tm_mday, (tmv.tm_year % 100));
    std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);

    std::string row = quote(severity) + "," + quote("http-nio-WebStrada-exec-1") + "," + quote(dateBuf) + "," + quote(timeBuf)
                       + "," + quote(appCol) + "," + quote(message) + "\r\n";

    std::ofstream f(path, std::ios::app);
    if (!f) {
        throw webstrada::exception("WriteLog: Unable to open log file " + webstrada::string(path.c_str()));
    }
    if (isNew) {
        f << "\"Severity\",\"ThreadID\",\"Date\",\"Time\",\"Application\",\"Message\"\r\n";
    }
    f << row;
    f.close();

    auto *ret = new cfvariant(string(""));
    // Written to by the <cflog> tag (emitCall) and directly by unit tests;
    // register the result so every caller frees it via the request cleanup.
    cf_register_temp(ret);
    return ret;
}

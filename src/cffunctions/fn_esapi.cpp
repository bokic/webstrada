/**
 * @file fn_esapi.cpp
 * @brief Shared OWASP-ESAPI-compatible encoder/decoder infrastructure.
 */

#include "fn_esapi.h"

#include <webstrada/exceptions.h>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace cfml {

bool esapiIsAlphanumeric(int ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
}

int esapiUtf8Next(const std::string &in, size_t &i) {
    unsigned char c = static_cast<unsigned char>(in[i]);
    if (c < 0x80) { i++; return c; }
    if ((c & 0xE0) == 0xC0 && i + 1 < in.size()) {
        int cp = ((c & 0x1F) << 6) | (in[i + 1] & 0x3F);
        i += 2;
        return cp;
    }
    if ((c & 0xF0) == 0xE0 && i + 2 < in.size()) {
        int cp = ((c & 0x0F) << 12) | ((in[i + 1] & 0x3F) << 6) | (in[i + 2] & 0x3F);
        i += 3;
        return cp;
    }
    if ((c & 0xF8) == 0xF0 && i + 3 < in.size()) {
        int cp = ((c & 0x07) << 18) | ((in[i + 1] & 0x3F) << 12) | ((in[i + 2] & 0x3F) << 6) | (in[i + 3] & 0x3F);
        i += 4;
        return cp;
    }
    i++;
    return c;
}

// ---- HTMLEntityCodec: HTML named-entity map (ESAPI mkCharacterToEntityMap). ----

static const std::map<int, const char *> &htmlEntityMap() {
    static const std::map<int, const char *> kMap = {
#include "esapi_entities.inc"
    };
    return kMap;
}

// Lowercase hex (no leading zeros) like Integer.toHexString.
static std::string toHexString(int v) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%x", v);
    return buf;
}

// AbstractCodec.getHexForNonAlphanumeric: for c < 255 returns null when the
// char is ASCII alphanumeric, else lowercase hex; for c >= 255 always hex.
// Returns true when the char must be encoded (hex non-null).
static bool esapiHexForCodePoint(int codePoint, std::string &hex) {
    if (codePoint < 255) {
        if (esapiIsAlphanumeric(codePoint)) return false;
        hex = toHexString(codePoint);
        return true;
    }
    hex = toHexString(codePoint);
    return true;
}

std::string esapiEncodeHtmlCodePoint(int codePoint, const char *immune, int immuneLen) {
    for (int i = 0; i < immuneLen; i++) {
        if (codePoint == static_cast<unsigned char>(immune[i])) {
            return std::string(1, static_cast<char>(codePoint));
        }
    }
    std::string hex;
    if (!esapiHexForCodePoint(codePoint, hex)) {
        return std::string(1, static_cast<char>(codePoint));
    }
    // C0 controls (except tab/LF/CR) and C1 range map to the replacement char.
    if ((codePoint <= 31 && codePoint != 9 && codePoint != 10 && codePoint != 13) ||
        (codePoint >= 127 && codePoint <= 159)) {
        hex = "fffd";
        codePoint = 0xFFFD;
    }
    auto it = htmlEntityMap().find(codePoint);
    if (it != htmlEntityMap().end()) {
        std::string out = "&";
        out += it->second;
        out += ";";
        return out;
    }
    return "&#x" + hex + ";";
}

std::string esapiEncodeXmlCodePoint(int codePoint, const char *immune, int immuneLen) {
    for (int i = 0; i < immuneLen; i++) {
        if (codePoint == static_cast<unsigned char>(immune[i])) {
            return std::string(1, static_cast<char>(codePoint));
        }
    }
    // UNENCODED_SET = alphanumeric + space + tab.
    if (esapiIsAlphanumeric(codePoint) || codePoint == ' ' || codePoint == '\t') {
        return std::string(1, static_cast<char>(codePoint));
    }
    return "&#x" + toHexString(codePoint) + ";";
}

std::string esapiEncodeJavaScriptCodePoint(int codePoint, const char *immune, int immuneLen) {
    for (int i = 0; i < immuneLen; i++) {
        if (codePoint == static_cast<unsigned char>(immune[i])) {
            return std::string(1, static_cast<char>(codePoint));
        }
    }
    std::string hex;
    if (!esapiHexForCodePoint(codePoint, hex)) {
        return std::string(1, static_cast<char>(codePoint));
    }
    std::string temp = toHexString(codePoint);
    std::string out;
    if (codePoint < 256) {
        out = "\\x";
        if (temp.length() == 1) out += "0";
        for (char c : temp) out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    } else {
        out = "\\u";
        while (temp.length() < 4) temp.insert(0, "0");
        for (char c : temp) out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

std::string esapiEncodeCssCodePoint(int codePoint, const char *immune, int immuneLen) {
    for (int i = 0; i < immuneLen; i++) {
        if (codePoint == static_cast<unsigned char>(immune[i])) {
            return std::string(1, static_cast<char>(codePoint));
        }
    }
    std::string hex;
    if (!esapiHexForCodePoint(codePoint, hex)) {
        return std::string(1, static_cast<char>(codePoint));
    }
    return "\\" + hex + " ";
}

std::string esapiEncodeString(const std::string &in,
                              std::string (*enc)(int, const char *, int),
                              const char *immune, int immuneLen) {
    std::string out;
    size_t i = 0;
    while (i < in.size()) {
        int cp = esapiUtf8Next(in, i);
        out += enc(cp, immune, immuneLen);
    }
    return out;
}

// ---- Decoders (PushbackString semantics simplified to plain strings). ----
// Numeric entity: parses an optional 'x'/'X' hex or decimal run, terminated by
// ';' or end-of-input. Returns true + the value on success.
static bool parseNumericEntity(const std::string &s, size_t &i, int &out) {
    // s[i] is the char after '&#' (or after '&#' when caller consumed it).
    bool isHex = false;
    if (i < s.size() && (s[i] == 'x' || s[i] == 'X')) {
        isHex = true;
        i++;
    }
    std::string digits;
    while (i < s.size()) {
        char c = s[i];
        if (c == ';') { i++; break; }
        if (isHex) {
            if (std::isxdigit(static_cast<unsigned char>(c))) {
                digits += c;
                i++;
            } else {
                break;
            }
        } else {
            if (std::isdigit(static_cast<unsigned char>(c))) {
                digits += c;
                i++;
            } else {
                break;
            }
        }
    }
    if (digits.empty()) return false;
    try {
        long v = isHex ? std::stol(digits, nullptr, 16) : std::stol(digits, nullptr, 10);
        if (v >= 0 && v <= 0x10FFFF) {
            out = static_cast<int>(v);
            return true;
        }
    } catch (...) {
    }
    return false;
}

// Named entity: longest case-insensitive prefix match against the entity map
// (ESAPI's HashTrie.getLongestMatch; keys are lowercase, match is
// case-insensitive). Consumes the matched name and an optional trailing ';'.
static bool matchNamedEntity(const std::string &s, size_t &i, int &out) {
    if (i >= s.size()) return false;
    const std::map<int, const char *> &map = htmlEntityMap();
    // Best match: scan all entity names (this is small; the real trie is an
    // optimization). Prefer the longest name that matches case-insensitively.
    size_t bestLen = 0;
    const char *bestName = nullptr;
    int bestValue = 0;
    for (const auto &kv : map) {
        size_t nameLen = std::strlen(kv.second);
        if (nameLen > bestLen && i + nameLen <= s.size()) {
            bool eq = true;
            for (size_t k = 0; k < nameLen; k++) {
                unsigned char a = static_cast<unsigned char>(s[i + k]);
                unsigned char b = static_cast<unsigned char>(kv.second[k]);
                if (std::tolower(a) != std::tolower(b)) { eq = false; break; }
            }
            if (eq) {
                bestLen = nameLen;
                bestName = kv.second;
                bestValue = kv.first;
            }
        }
    }
    if (!bestName) return false;
    // ESAPI consumes the matched key then an optional ';'.
    i += bestLen;
    if (i < s.size() && s[i] == ';') i++;
    out = bestValue;
    return true;
}

// HTMLEntityCodec.decodeCharacter over a plain string. `start` is the index of
// the '&'. Returns false when the entity is malformed (caller emits '&' raw).
static bool decodeHtmlEntity(const std::string &s, size_t start, size_t &end, int &out) {
    if (start + 1 >= s.size()) return false;
    char second = s[start + 1];
    size_t i = start + 1;
    if (second == '#') {
        i++;
        if (parseNumericEntity(s, i, out)) {
            end = i;
            return true;
        }
        return false;
    }
    if (std::isalpha(static_cast<unsigned char>(second))) {
        if (matchNamedEntity(s, i, out)) {
            end = i;
            return true;
        }
        return false;
    }
    return false;
}

std::string esapiDecodeHtml(const std::string &input) {
    std::string out;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '&') {
            size_t end = 0;
            int v = 0;
            if (decodeHtmlEntity(input, i, end, v)) {
                if (v <= 0x7F) out += static_cast<char>(v);
                else {
                    // Encode as UTF-8 (code points > U+007F).
                    if (v <= 0x7FF) {
                        out += static_cast<char>(0xC0 | (v >> 6));
                        out += static_cast<char>(0x80 | (v & 0x3F));
                    } else if (v <= 0xFFFF) {
                        out += static_cast<char>(0xE0 | (v >> 12));
                        out += static_cast<char>(0x80 | ((v >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (v & 0x3F));
                    } else {
                        out += static_cast<char>(0xF0 | (v >> 18));
                        out += static_cast<char>(0x80 | ((v >> 12) & 0x3F));
                        out += static_cast<char>(0x80 | ((v >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (v & 0x3F));
                    }
                }
                i = end;
                continue;
            }
        }
        out += input[i];
        i++;
    }
    return out;
}

// XMLEntityCodec.decode — only the five named entities + numeric entities.
static bool decodeXmlEntity(const std::string &s, size_t start, size_t &end, int &out) {
    if (start + 1 >= s.size()) return false;
    char second = s[start + 1];
    size_t i = start + 1;
    if (second == '#') {
        i++;
        if (parseNumericEntity(s, i, out)) {
            end = i;
            return true;
        }
        return false;
    }
    if (std::isalpha(static_cast<unsigned char>(second))) {
        // Named entities: lt gt amp apos quot. Longest match (case-insensitive).
        static const char *kNames[5] = {"amp", "apos", "gt", "lt", "quot"};
        static const int kValues[5] = {'&', '\'', '>', '<', '"'};
        size_t bestLen = 0;
        int bestVal = 0;
        for (int k = 0; k < 5; k++) {
            size_t nLen = std::strlen(kNames[k]);
            if (nLen > bestLen && i + nLen <= s.size()) {
                bool eq = true;
                for (size_t j = 0; j < nLen; j++) {
                    if (std::tolower(static_cast<unsigned char>(s[i + j])) !=
                        static_cast<unsigned char>(kNames[k][j])) { eq = false; break; }
                }
                if (eq) { bestLen = nLen; bestVal = kValues[k]; }
            }
        }
        if (bestLen == 0) return false;
        // XML named entity requires a following ';' (XML strictly requires it).
        if (i + bestLen >= s.size() || s[i + bestLen] != ';') return false;
        i += bestLen + 1;
        end = i;
        out = bestVal;
        return true;
    }
    return false;
}

std::string esapiDecodeXml(const std::string &input) {
    std::string out;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '&') {
            size_t end = 0;
            int v = 0;
            if (decodeXmlEntity(input, i, end, v)) {
                if (v <= 0x7F) out += static_cast<char>(v);
                else if (v <= 0x7FF) {
                    out += static_cast<char>(0xC0 | (v >> 6));
                    out += static_cast<char>(0x80 | (v & 0x3F));
                } else {
                    out += static_cast<char>(0xE0 | (v >> 12));
                    out += static_cast<char>(0x80 | ((v >> 6) & 0x3F));
                    out += static_cast<char>(0x80 | (v & 0x3F));
                }
                i = end;
                continue;
            }
        }
        out += input[i];
        i++;
    }
    return out;
}

// PercentCodec.decodeCharacter: '%' + exactly two hex digits.
std::string esapiDecodePercent(const std::string &input) {
    std::string out;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '%' && i + 2 < input.size() &&
            std::isxdigit(static_cast<unsigned char>(input[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(input[i + 2]))) {
            unsigned int v = 0;
            std::sscanf(input.c_str() + i + 1, "%2x", &v);
            out += static_cast<char>(v);
            i += 3;
            continue;
        }
        out += input[i];
        i++;
    }
    return out;
}

// JavaScriptCodec.decodeCharacter: \b \t \n \v \f \r \" \' \\ \xHH \uHHHH
// \NNN (octal, up to 3 digits).
std::string esapiDecodeJavaScript(const std::string &input) {
    std::string out;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char d = input[i + 1];
            if (d == 'b') { out += '\b'; i += 2; continue; }
            if (d == 't') { out += '\t'; i += 2; continue; }
            if (d == 'n') { out += '\n'; i += 2; continue; }
            if (d == 'v') { out += static_cast<char>(11); i += 2; continue; }
            if (d == 'f') { out += '\f'; i += 2; continue; }
            if (d == 'r') { out += '\r'; i += 2; continue; }
            if (d == '"' || d == '\'' || d == '\\') { out += d; i += 2; continue; }
            if (d == 'x' || d == 'X') {
                if (i + 3 < input.size() &&
                    std::isxdigit(static_cast<unsigned char>(input[i + 2])) &&
                    std::isxdigit(static_cast<unsigned char>(input[i + 3]))) {
                    unsigned int v = 0;
                    std::sscanf(input.c_str() + i + 2, "%2x", &v);
                    out += static_cast<char>(v);
                    i += 4;
                    continue;
                }
            } else if (d == 'u' || d == 'U') {
                if (i + 5 < input.size()) {
                    bool ok = true;
                    for (int k = 0; k < 4; k++) {
                        if (!std::isxdigit(static_cast<unsigned char>(input[i + 2 + k]))) { ok = false; break; }
                    }
                    if (ok) {
                        unsigned int v = 0;
                        std::sscanf(input.c_str() + i + 2, "%4x", &v);
                        if (v <= 0x7F) out += static_cast<char>(v);
                        else if (v <= 0x7FF) {
                            out += static_cast<char>(0xC0 | (v >> 6));
                            out += static_cast<char>(0x80 | (v & 0x3F));
                        } else {
                            out += static_cast<char>(0xE0 | (v >> 12));
                            out += static_cast<char>(0x80 | ((v >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (v & 0x3F));
                        }
                        i += 6;
                        continue;
                    }
                }
            } else if (d >= '0' && d <= '7') {
                // Octal escape: up to three octal digits.
                std::string octal;
                octal += d;
                size_t k = i + 2;
                while (k < input.size() && octal.length() < 3 &&
                       input[k] >= '0' && input[k] <= '7') {
                    octal += input[k];
                    k++;
                }
                int v = std::stoi(octal, nullptr, 8);
                out += static_cast<char>(v);
                i = k;
                continue;
            }
        }
        out += input[i];
        i++;
    }
    return out;
}

// ---- Canonicalize ----

std::string esapiCanonicalize(const std::string &input,
                              bool restrictMultiple, bool restrictMixed) {
    std::string working = input;
    int mixedCount = 1;
    int foundCount = 0;
    int prevCodec = -1; // 0=html, 1=percent, 2=js
    bool clean = false;
    while (!clean) {
        clean = true;
        for (int codec = 0; codec < 3; codec++) {
            std::string old = working;
            if (codec == 0) working = esapiDecodeHtml(working);
            else if (codec == 1) working = esapiDecodePercent(working);
            else working = esapiDecodeJavaScript(working);
            if (old != working) {
                if (prevCodec != -1 && prevCodec != codec) mixedCount++;
                prevCodec = codec;
                if (clean) foundCount++;
                clean = false;
            }
        }
    }
    if (foundCount >= 2 && mixedCount > 1) {
        if (restrictMultiple || restrictMixed) {
            throw webstrada::exception("org.owasp.esapi.errors.IntrusionException",
                                      "Input validation failure", "");
        }
    } else if (foundCount >= 2) {
        if (restrictMultiple) {
            throw webstrada::exception("org.owasp.esapi.errors.IntrusionException",
                                      "Input validation failure", "");
        }
    } else if (mixedCount > 1) {
        if (restrictMixed) {
            throw webstrada::exception("org.owasp.esapi.errors.IntrusionException",
                                      "Input validation failure", "");
        }
    }
    return working;
}

std::string esapiCanonicalizeCatch(const std::string &input,
                                   bool restrictMultiple, bool restrictMixed,
                                   bool throwOnError) {
    if (input.empty()) return input;
    try {
        return esapiCanonicalize(input, restrictMultiple, restrictMixed);
    } catch (...) {
        if (!throwOnError) return "";
        throw;
    }
}

// ---- DN / LDAP ----

// Encode one char >= U+0080 as its UTF-8 bytes each "\\%02x".
static void appendUtf8HexBytes(const std::string &ch, std::string &out) {
    // ch is a UTF-8 byte sequence for a single code point.
    for (unsigned char b : ch) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\%02x", b);
        out += buf;
    }
}

std::string esapiEncodeDn(const std::string &input) {
    std::string out;
    if (!input.empty() && (input[0] == ' ' || input[0] == '#')) out += '\\';
    size_t i = 0;
    while (i < input.size()) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c < 0x80) {
            switch (c) {
            case 0: out += "\\00"; break;
            case '"': out += "\\\""; break;
            case '+': out += "\\+"; break;
            case ',': out += "\\,"; break;
            case '/': out += "\\/"; break;
            case ';': out += "\\;"; break;
            case '<': out += "\\<"; break;
            case '>': out += "\\>"; break;
            case '\\': out += "\\\\"; break;
            default: out += static_cast<char>(c); break;
            }
            i++;
        } else {
            // Decode the UTF-8 sequence for one code point.
            size_t len = 1;
            if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;
            std::string seq = input.substr(i, len);
            appendUtf8HexBytes(seq, out);
            i += len;
        }
    }
    if (input.length() > 1 && input.back() == ' ') {
        // insert '\' before the trailing space
        size_t pos = out.length() - 1;
        out.insert(pos, "\\");
    }
    return out;
}

std::string esapiEncodeLdap(const std::string &input) {
    std::string out;
    size_t i = 0;
    while (i < input.size()) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c < 0x80) {
            switch (c) {
            case 0: out += "\\00"; break;
            case '(': out += "\\28"; break;
            case ')': out += "\\29"; break;
            case '*': out += "\\2a"; break;
            case '/': out += "\\2f"; break;
            case '\\': out += "\\5c"; break;
            default: out += static_cast<char>(c); break;
            }
            i++;
        } else {
            size_t len = 1;
            if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;
            std::string seq = input.substr(i, len);
            appendUtf8HexBytes(seq, out);
            i += len;
        }
    }
    return out;
}

} // namespace cfml

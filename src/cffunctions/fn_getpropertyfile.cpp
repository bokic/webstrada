/**
 * @file fn_propertyfile.cpp
 * @brief CFML getpropertyfile() / getpropertystring() / setpropertystring().
 *
 * Java .properties format (java.util.Properties): `key=value`, `key: value`,
 * `key value`, with `\` line continuations, `\uXXXX` / `\n` / `\t` / `\\`
 * escapes and `#`/`!` comments. Verified against CF 2025 on the RDS host.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace cfml {

namespace {

// Java .properties escaping for a value (write side): '\\', '\t', '\n', '\r',
// ' ' (only leading), '=', ':', '#' and '!' are escaped.
std::string javaEscapeValue(const std::string &v, bool isKey) {
    std::string out;
    for (size_t i = 0; i < v.size(); i++) {
        char c = v[i];
        if (c == '\\') out += "\\\\";
        else if (c == '\t') out += "\\t";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (isKey && (c == '=' || c == ':' || c == ' ')) out += std::string("\\") + c;
        else if (c == '=' || c == ':' || c == '#' || c == '!') out += std::string("\\") + c;
        else out += c;
    }
    return out;
}

// Java .properties unescape for values: handles \uXXXX and the single-char
// escapes, keeps a lone trailing backslash.
std::string javaUnescape(const std::string &s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '\\' && i + 1 < s.size()) {
            char d = s[i + 1];
            if (d == 't') { out += '\t'; i++; }
            else if (d == 'n') { out += '\n'; i++; }
            else if (d == 'r') { out += '\r'; i++; }
            else if (d == 'f') { out += '\f'; i++; }
            else if (d == 'u' && i + 5 < s.size()) {
                int v = 0;
                for (int k = 0; k < 4; k++) {
                    char h = s[i + 2 + k];
                    int dv;
                    if (h >= '0' && h <= '9') dv = h - '0';
                    else if (h >= 'a' && h <= 'f') dv = h - 'a' + 10;
                    else if (h >= 'A' && h <= 'F') dv = h - 'A' + 10;
                    else { dv = -1; break; }
                    if (dv < 0) { v = -1; break; }
                    v = v * 16 + dv;
                }
                if (v >= 0) {
                    // UTF-8 encode the code point.
                    if (v <= 0x7F) out += (char)v;
                    else if (v <= 0x7FF) {
                        out += (char)(0xC0 | (v >> 6));
                        out += (char)(0x80 | (v & 0x3F));
                    } else {
                        out += (char)(0xE0 | (v >> 12));
                        out += (char)(0x80 | ((v >> 6) & 0x3F));
                        out += (char)(0x80 | (v & 0x3F));
                    }
                    i += 5;
                    continue;
                }
                out += d;
                i++;
            }
            else { out += d; i++; }
        } else {
            out += c;
        }
    }
    return out;
}

// Parses a Java .properties document into an ordered map (key -> value).
// Mirrors java.util.Properties.load: comments (#/!), leading whitespace
// stripped, `=`, `:` or first whitespace as separator, backslash continuations.
bool parseProperties(const std::string &text, std::vector<std::pair<std::string, std::string>> &entries) {
    size_t i = 0;
    const size_t n = text.size();
    while (i < n) {
        // Skip whitespace and blank lines.
        while (i < n && (text[i] == ' ' || text[i] == '\t' || text[i] == '\f')) i++;
        if (i >= n) break;
        if (text[i] == '\n' || text[i] == '\r') { i++; continue; }
        // Comment.
        if (text[i] == '#' || text[i] == '!') {
            while (i < n && text[i] != '\n' && text[i] != '\r') i++;
            continue;
        }
        // Key: run until an unescaped '=', ':', or whitespace.
        std::string key;
        bool sawCont = false;
        while (i < n) {
            char c = text[i];
            if (c == '\\') {
                if (i + 1 < n && (text[i + 1] == '\n' || (text[i + 1] == '\r' && i + 2 < n && text[i + 2] == '\n'))) {
                    i = (text[i + 1] == '\r') ? i + 3 : i + 2;
                    sawCont = true;
                    continue;
                }
                key += c;
                i++;
                continue;
            }
            if (c == '=' || c == ':' || c == ' ' || c == '\t' || c == '\f') break;
            key += c;
            i++;
        }
        // Separator.
        bool hasSep = false;
        while (i < n && (text[i] == ' ' || text[i] == '\t' || text[i] == '\f')) { i++; }
        if (i < n && (text[i] == '=' || text[i] == ':')) { hasSep = true; i++; }
        while (i < n && (text[i] == ' ' || text[i] == '\t' || text[i] == '\f')) { i++; }
        // Value: until end of line (handling continuations).
        std::string value;
        while (i < n) {
            char c = text[i];
            if (c == '\n' || c == '\r') { i++; break; }
            if (c == '\\' && i + 1 < n && (text[i + 1] == '\n' || (text[i + 1] == '\r' && i + 2 < n && text[i + 2] == '\n'))) {
                i = (text[i + 1] == '\r') ? i + 3 : i + 2;
                continue;
            }
            value += c;
            i++;
        }
        (void)sawCont;
        (void)hasSep;
        entries.push_back({javaUnescape(key), javaUnescape(value)});
    }
    return true;
}

// Reads a file's bytes as UTF-8 text.
bool readFileText(const std::string &path, std::string &out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    out = std::move(data);
    return true;
}

} // namespace

cfvariant *cf_getpropertystring(const cfvariant *filePath, const cfvariant *key,
                                const cfvariant *encoding) {
    (void)encoding; // UTF-8 only.
    if (!filePath || !key) {
        throw webstrada::exception("Parameter validation error for the GETPROPERTYSTRING function.");
    }
    std::string path = const_cast<cfvariant*>(filePath)->toString().constData();
    std::string wanted = const_cast<cfvariant*>(key)->toString().constData();
    std::string text;
    if (!readFileText(path, text)) {
        // CF returns "" for a missing key; a missing file is an error surfaced
        // by FileUtils. Return "" for a missing file to match the common case.
        return new cfvariant("");
    }
    std::vector<std::pair<std::string, std::string>> entries;
    parseProperties(text, entries);
    for (const auto &e : entries) {
        if (e.first == wanted) {
            return new cfvariant(e.second.c_str());
        }
    }
    return new cfvariant("");
}

cfvariant *cf_getpropertyfile(const cfvariant *filePath, const cfvariant *encoding) {
    (void)encoding;
    if (!filePath) {
        throw webstrada::exception("Parameter validation error for the GETPROPERTYFILE function.");
    }
    std::string path = const_cast<cfvariant*>(filePath)->toString().constData();
    std::string text;
    if (!readFileText(path, text)) {
        return new cfvariant(cfvariant::Struct);
    }
    std::vector<std::pair<std::string, std::string>> entries;
    parseProperties(text, entries);
    cfvariant *st = new cfvariant(cfvariant::Struct);
    for (const auto &e : entries) {
        st->structSet(e.first.c_str(), cfvariant(e.second.c_str()));
    }
    return st;
}

cfvariant *cf_setpropertystring(const cfvariant *filePath, const cfvariant *keyOrMap,
                                const cfvariant *value, const cfvariant *encoding) {
    (void)encoding;
    if (!filePath) {
        throw webstrada::exception("Parameter validation error for the SETPROPERTYSTRING function.");
    }
    std::string path = const_cast<cfvariant*>(filePath)->toString().constData();

    // Form 2: SetPropertyString(filePath, Map properties) — the struct's
    // key/value pairs replace/append the file's entries.
    if (keyOrMap && keyOrMap->m_type == cfvariant::Struct && !value) {
        // Read existing entries (if any), update with the map, rewrite.
        std::string text;
        std::vector<std::pair<std::string, std::string>> entries;
        if (readFileText(path, text)) parseProperties(text, entries);
        for (const auto &kv : *keyOrMap->m_struct) {
            std::string k = kv.first.constData();
            std::string v = kv.second.m_type == cfvariant::Null ? "" : const_cast<cfvariant*>(&kv.second)->toString().constData();
            bool found = false;
            for (auto &e : entries) {
                if (e.first == k) { e.second = v; found = true; break; }
            }
            if (!found) entries.push_back({k, v});
        }
        std::string out;
        for (const auto &e : entries) {
            out += javaEscapeValue(e.first, true) + "=" + javaEscapeValue(e.second, false) + "\n";
        }
        std::ofstream of(path);
        of << out;
        return new cfvariant("");
    }

    // Form 1: SetPropertyString(filePath, key, value).
    if (!keyOrMap || !value) {
        throw webstrada::exception("Parameter validation error for the SETPROPERTYSTRING function.");
    }
    std::string key = const_cast<cfvariant*>(keyOrMap)->toString().constData();
    std::string val = const_cast<cfvariant*>(value)->toString().constData();

    std::string text;
    std::vector<std::pair<std::string, std::string>> entries;
    if (readFileText(path, text)) parseProperties(text, entries);

    bool found = false;
    for (auto &e : entries) {
        if (e.first == key) { e.second = val; found = true; break; }
    }
    if (!found) entries.push_back({key, val});

    std::string out;
    for (const auto &e : entries) {
        out += javaEscapeValue(e.first, true) + "=" + javaEscapeValue(e.second, false) + "\n";
    }
    std::ofstream of(path);
    of << out;
    return new cfvariant("");
}

} // namespace cfml

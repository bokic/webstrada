/**
 * @file fn_setprofilestring.cpp
 * @brief CFML setprofilestring() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_setprofilestring(const cfvariant *path, const cfvariant *section,
                               const cfvariant *entry, const cfvariant *value,
                               const cfvariant *encoding) {
    (void)encoding; // ASCII/UTF-8 only; the encoding argument is accepted but unused.
    if (!path || !section || !entry || !value) {
        throw webstrada::exception("SetProfileString requires at least 4 arguments");
    }
    std::string filename = const_cast<cfvariant*>(path)->toString().constData();
    std::string sec = const_cast<cfvariant*>(section)->toString().constData();
    std::string key = const_cast<cfvariant*>(entry)->toString().constData();
    std::string val = const_cast<cfvariant*>(value)->toString().constData();

    std::ifstream infile(filename);
    if (!infile) {
        // CF: creating a missing file with a new section.
        std::ofstream ofile(filename);
        ofile << "[" << sec << "]\n" << key << "=" << val << "\n";
        return new cfvariant("");
    }

    // Parse into ordered sections: name -> list of (comment, origKey, value,
    // cleanKeyLower). Comments (lines starting with ';' or '#' without '=')
    // directly preceding a property are attached to it; comments after the last
    // property become a closing comment (preserved but not replayed here for
    // simplicity — CF carries them, which rarely matters).
    struct Entry { std::string comment; std::string origKey; std::string value; std::string cleanKey; };
    std::vector<std::pair<std::string, std::vector<Entry>>> sections;
    std::vector<Entry> *cur = nullptr;
    std::string pendingComment;

    auto trimWs = [](std::string s) {
        size_t b = s.find_first_not_of(" \t\n\r\f");
        if (b == std::string::npos) return std::string();
        size_t e = s.find_last_not_of(" \t\n\r\f");
        return s.substr(b, e - b + 1);
    };

    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string t = trimWs(line);
        if (t.empty()) continue;
        // Comment line: ';' or '#' without '='.
        if (t[0] == ';' || (t[0] == '#' && t.find('=') == std::string::npos)) {
            pendingComment += line;
            continue;
        }
        // Section header.
        if (t[0] == '[' && t.back() == ']') {
            std::string name = t.substr(1, t.size() - 2);
            sections.push_back({name, {}});
            cur = &sections.back().second;
            continue;
        }
        // Property line: key=value (only '=' delimits; a '#' with '=' is a key).
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string dirtyKey = line.substr(0, eq);
        std::string valueStr = line.substr(eq + 1);
        std::string cleanKey = dirtyKey;
        for (auto &c : cleanKey) c = (char)tolower((unsigned char)c);
        cleanKey = trimWs(cleanKey);
        if (cur) {
            cur->push_back({pendingComment, dirtyKey, valueStr, cleanKey});
        }
        pendingComment.clear();
    }

    // Apply the update: find (or create) the section and set/replace the key,
    // preserving insertion order (new keys are appended).
    bool foundSec = false;
    for (auto &secE : sections) {
        if (secE.first == sec) {
            foundSec = true;
            bool foundKey = false;
            for (auto &e : secE.second) {
                if (e.cleanKey == key) {
                    e.value = val;
                    foundKey = true;
                    break;
                }
            }
            if (!foundKey) {
                secE.second.push_back({"", key, val, key});
            }
            break;
        }
    }
    if (!foundSec) {
        sections.push_back({sec, {{"", key, val, key}}});
    }

    // Rewrite the whole file.
    std::string out;
    for (const auto &secE : sections) {
        out += "[" + secE.first + "]\n";
        for (const auto &e : secE.second) {
            if (!e.comment.empty()) out += e.comment + "\n";
            out += e.origKey + "=" + e.value + "\n";
        }
    }
    std::ofstream ofile(filename);
    ofile << out;

    // CF returns "" on success (SetProfileString returns "").
    return new cfvariant("");
}

} // namespace cfml

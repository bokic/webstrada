/**
 * @file fn_getprofilestring.cpp
 * @brief CFML getprofilestring() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/upload.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

using webstrada::UploadedFile;

namespace cfml {

cfvariant *cf_getprofilestring(const cfvariant *iniPath, const cfvariant *section, const cfvariant *key) {
    if (!iniPath || !section || !key) throw webstrada::exception("GetProfileString: Missing argument(s)");
    string pathStr = const_cast<cfvariant*>(iniPath)->toString();
    string sectionStr = const_cast<cfvariant*>(section)->toString();
    string keyStr = const_cast<cfvariant*>(key)->toString();

    std::ifstream infile(pathStr.constData());
    if (!infile) throw webstrada::exception("GetProfileString: Failed to open file: " + pathStr);

    string currentSection;
    std::string line;
    while (std::getline(infile, line)) {
        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r");
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed.size() >= 2 && trimmed[0] == '[' && trimmed[trimmed.size() - 1] == ']') {
            currentSection = string(trimmed.substr(1, trimmed.size() - 2).c_str());
            continue;
        }

        if (!currentSection.isEmpty() && currentSection.equals(sectionStr)) {
            size_t eqPos = trimmed.find('=');
            if (eqPos != std::string::npos) {
                std::string k = trimmed.substr(0, eqPos);
                std::string v = trimmed.substr(eqPos + 1);
                // Trim key
                size_t kend = k.find_last_not_of(" \t");
                if (kend != std::string::npos) k = k.substr(0, kend + 1);
                if (string(k.c_str()).equals(keyStr)) {
                    string val(v.c_str());
                    auto *ret = new cfvariant(val);
                    return ret;
                }
            }
        }
    }

    // Key not found, return empty string
    auto *ret = new cfvariant("");
    return ret;
}

} // namespace cfml

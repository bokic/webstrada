/**
 * @file fn_getprofilesections.cpp
 * @brief CFML getprofilesections() built-in.
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

cfvariant *cf_getprofilesections(const cfvariant *iniPath) {
    if (!iniPath) throw webstrada::exception("GetProfileSections: Missing argument");
    string pathStr = const_cast<cfvariant*>(iniPath)->toString();

    std::ifstream infile(pathStr.constData());
    if (!infile) throw webstrada::exception("GetProfileSections: Failed to open file: " + pathStr);

    string sections;
    std::string line;
    while (std::getline(infile, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r");
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed.size() >= 2 && trimmed[0] == '[' && trimmed[trimmed.size() - 1] == ']') {
            string sectionName(trimmed.substr(1, trimmed.size() - 2).c_str());
            if (!sections.isEmpty()) sections += "\n";
            sections += sectionName;
        }
    }

    auto *ret = new cfvariant(sections);
    return ret;
}

} // namespace cfml

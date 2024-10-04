/**
 * @file fn_fileread.cpp
 * @brief CFML fileread() built-in.
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

cfvariant *cf_fileread(const cfvariant *path) {
    if (!path) throw webstrada::exception("FileRead: Missing argument");
    string pStr = const_cast<cfvariant*>(path)->toString();
    std::ifstream infile(pStr.constData());
    if (!infile) throw webstrada::exception("FileRead: Failed to open file: " + pStr);
    std::stringstream buffer;
    buffer << infile.rdbuf();
    auto *ret = new cfvariant(buffer.str().c_str());
    return ret;
}

} // namespace cfml

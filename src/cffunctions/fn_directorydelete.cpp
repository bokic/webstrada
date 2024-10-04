/**
 * @file fn_directorydelete.cpp
 * @brief CFML directorydelete() built-in.
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

cfvariant *cf_directorydelete(const cfvariant *path, const cfvariant *recurse) {
    if (!path) throw webstrada::exception("DirectoryDelete: Missing argument");
    string pStr = const_cast<cfvariant*>(path)->toString();
    bool rec = recurse ? isTruthy(*recurse) : false;
    if (rec) {
        std::filesystem::remove_all(pStr.constData());
    } else {
        std::filesystem::remove(pStr.constData());
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml

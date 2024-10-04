/**
 * @file fn_directorycreate.cpp
 * @brief CFML directorycreate() built-in.
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

cfvariant *cf_directorycreate(const cfvariant *path) {
    if (!path) throw webstrada::exception("DirectoryCreate: Missing argument");
    string pStr = const_cast<cfvariant*>(path)->toString();
    std::filesystem::create_directories(pStr.constData());
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml

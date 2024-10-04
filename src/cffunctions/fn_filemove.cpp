/**
 * @file fn_filemove.cpp
 * @brief CFML filemove() built-in.
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

cfvariant *cf_filemove(const cfvariant *src, const cfvariant *dest) {
    if (!src || !dest) throw webstrada::exception("FileMove: Missing argument(s)");
    string srcStr = const_cast<cfvariant*>(src)->toString();
    string destStr = const_cast<cfvariant*>(dest)->toString();
    std::filesystem::rename(srcStr.constData(), destStr.constData());
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml

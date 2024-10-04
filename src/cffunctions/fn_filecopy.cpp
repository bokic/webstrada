/**
 * @file fn_filecopy.cpp
 * @brief CFML filecopy() built-in.
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

cfvariant *cf_filecopy(const cfvariant *src, const cfvariant *dest) {
    if (!src || !dest) throw webstrada::exception("FileCopy: Missing argument(s)");
    string srcStr = const_cast<cfvariant*>(src)->toString();
    string destStr = const_cast<cfvariant*>(dest)->toString();
    std::filesystem::copy_file(srcStr.constData(), destStr.constData(), std::filesystem::copy_options::overwrite_existing);
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml

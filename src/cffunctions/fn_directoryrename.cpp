/**
 * @file fn_directoryrename.cpp
 * @brief CFML directoryrename() built-in.
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

cfvariant *cf_directoryrename(const cfvariant *source, const cfvariant *destination) {
    if (!source || !destination) throw webstrada::exception("DirectoryRename: Missing argument(s)");
    string srcStr = const_cast<cfvariant*>(source)->toString();
    string destStr = const_cast<cfvariant*>(destination)->toString();
    std::filesystem::rename(srcStr.constData(), destStr.constData());
    return cfvariant_create_bool(true);
}

} // namespace cfml

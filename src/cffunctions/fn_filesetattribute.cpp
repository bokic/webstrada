/**
 * @file fn_filesetattribute.cpp
 * @brief CFML filesetattribute() built-in.
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

cfvariant *cf_filesetattribute(const cfvariant *path, const cfvariant *attr) {
    if (!path || !attr) throw webstrada::exception("FileSetAttribute: Missing argument(s)");
    // On Linux, just return true (Windows-only functionality)
    return cfvariant_create_bool(true);
}

} // namespace cfml

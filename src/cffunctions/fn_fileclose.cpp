/**
 * @file fn_fileclose.cpp
 * @brief CFML fileclose() built-in.
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

cfvariant *cf_fileclose(const cfvariant *fileObj) {
    if (!fileObj) throw webstrada::exception("FileClose: Missing argument");
    if (fileObj->m_type != cfvariant::File) throw webstrada::exception("FileClose: Argument is not a file object");
    if (fileObj->m_fd > 2) {
        close(fileObj->m_fd);
        const_cast<cfvariant*>(fileObj)->m_fd = -1;
    }
    return cfvariant_create_bool(true);
}

} // namespace cfml

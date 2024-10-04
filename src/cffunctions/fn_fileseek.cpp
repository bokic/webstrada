/**
 * @file fn_fileseek.cpp
 * @brief CFML fileseek() built-in.
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

cfvariant *cf_fileseek(const cfvariant *fileObj, const cfvariant *position) {
    if (!fileObj || !position) throw webstrada::exception("FileSeek: Missing argument(s)");
    if (fileObj->m_type != cfvariant::File) throw webstrada::exception("FileSeek: Argument is not a file object");
    int fd = fileObj->m_fd;
    if (fd < 0) throw webstrada::exception("FileSeek: File is not open");

    off_t pos = getIntValue(*position);
    lseek(fd, pos, SEEK_SET);
    return cfvariant_create_bool(true);
}

} // namespace cfml

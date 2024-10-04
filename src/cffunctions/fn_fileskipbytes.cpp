/**
 * @file fn_fileskipbytes.cpp
 * @brief CFML fileskipbytes() built-in.
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

cfvariant *cf_fileskipbytes(const cfvariant *fileObj, const cfvariant *count) {
    if (!fileObj || !count) throw webstrada::exception("FileSkipBytes: Missing argument(s)");
    if (fileObj->m_type != cfvariant::File) throw webstrada::exception("FileSkipBytes: Argument is not a file object");
    int fd = fileObj->m_fd;
    if (fd < 0) throw webstrada::exception("FileSkipBytes: File is not open");

    off_t cnt = getIntValue(*count);
    lseek(fd, cnt, SEEK_CUR);
    return cfvariant_create_bool(true);
}

} // namespace cfml

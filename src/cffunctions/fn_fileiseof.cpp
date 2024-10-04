/**
 * @file fn_fileiseof.cpp
 * @brief CFML fileiseof() built-in.
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

cfvariant *cf_fileiseof(const cfvariant *fileObj) {
    if (!fileObj) throw webstrada::exception("FileIsEOF: Missing argument");
    if (fileObj->m_type != cfvariant::File) throw webstrada::exception("FileIsEOF: Argument is not a file object");

    int fd = fileObj->m_fd;
    if (fd < 0) throw webstrada::exception("FileIsEOF: File is not open");

    off_t cur = lseek(fd, 0, SEEK_CUR);
    off_t end = lseek(fd, 0, SEEK_END);
    lseek(fd, cur, SEEK_SET);

    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = (cur >= end);
    return ret;
}

} // namespace cfml

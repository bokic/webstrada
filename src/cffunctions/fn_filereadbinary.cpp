/**
 * @file fn_filereadbinary.cpp
 * @brief CFML filereadbinary() built-in.
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

cfvariant *cf_filereadbinary(const cfvariant *path) {
    if (!path) throw webstrada::exception("FileReadBinary: Missing argument");

    int fd = -1;
    bool closeAfter = false;
    if (path->m_type == cfvariant::File) {
        fd = path->m_fd;
        if (fd < 0) throw webstrada::exception("FileReadBinary: File is not open");
    } else {
        string pStr = const_cast<cfvariant*>(path)->toString();
        fd = open(pStr.constData(), O_RDONLY);
        if (fd < 0) throw webstrada::exception("FileReadBinary: Failed to open file: " + pStr);
        closeAfter = true;
    }

    off_t cur = lseek(fd, 0, SEEK_CUR);
    off_t endPos = lseek(fd, 0, SEEK_END);
    off_t size = endPos - cur;
    lseek(fd, cur, SEEK_SET);

    auto *ret = new cfvariant(cfvariant::Binary);
    ret->m_binary->resize(size);
    if (size > 0)
        read(fd, ret->m_binary->data(), size);

    if (closeAfter) close(fd);
    return ret;
}

} // namespace cfml

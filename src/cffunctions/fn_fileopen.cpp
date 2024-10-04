/**
 * @file fn_fileopen.cpp
 * @brief CFML fileopen() built-in.
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

cfvariant *cf_fileopen(const cfvariant *path, const cfvariant *mode, const cfvariant *charset) {
    if (!path || !mode) throw webstrada::exception("FileOpen: Missing argument(s)");
    string pStr = const_cast<cfvariant*>(path)->toString();
    string mStr = const_cast<cfvariant*>(mode)->toString();
    mStr.toUpper();

    int flags = 0;
    if (mStr.equals("READ")) {
        flags = O_RDONLY;
    } else if (mStr.equals("WRITE")) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (mStr.equals("APPEND")) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else {
        throw webstrada::exception("FileOpen: Invalid mode '" + mStr + "'. Valid modes: read, write, append");
    }

    int fd = open(pStr.constData(), flags, 0644);
    if (fd == -1) {
        throw webstrada::exception("FileOpen: Failed to open file: " + pStr);
    }

    auto *ret = new cfvariant(cfvariant::File);
    ret->m_fd = fd;
    return ret;
}

} // namespace cfml

/**
 * @file fn_filereadline.cpp
 * @brief CFML filereadline() built-in.
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

cfvariant *cf_filereadline(const cfvariant *fileObj) {
    if (!fileObj) throw webstrada::exception("FileReadLine: Missing argument");
    if (fileObj->m_type != cfvariant::File) throw webstrada::exception("FileReadLine: Argument is not a file object");
    int fd = fileObj->m_fd;
    if (fd < 0) throw webstrada::exception("FileReadLine: File is not open");

    string line;
    char ch;
    while (read(fd, &ch, 1) == 1) {
        if (ch == '\n') break;
        if (ch == '\r') {
            // check for \r\n
            char next;
            if (read(fd, &next, 1) == 1 && next != '\n') {
                lseek(fd, -1, SEEK_CUR);
            }
            break;
        }
        line += ch;
    }

    // Track end-of-file: restore position after comparing
    off_t curPos = lseek(fd, 0, SEEK_CUR);
    off_t endPos = lseek(fd, 0, SEEK_END);
    lseek(fd, curPos, SEEK_SET);
    if (line.isEmpty() && curPos >= endPos) {
        throw webstrada::exception("FileReadLine: End of file reached");
    }

    auto *ret = new cfvariant(line);
    return ret;
}

} // namespace cfml

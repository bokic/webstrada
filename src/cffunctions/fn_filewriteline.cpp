/**
 * @file fn_filewriteline.cpp
 * @brief CFML filewriteline() built-in.
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

cfvariant *cf_filewriteline(const cfvariant *fileObj, const cfvariant *content) {
    if (!fileObj || !content) throw webstrada::exception("FileWriteLine: Missing argument(s)");
    if (fileObj->m_type != cfvariant::File) throw webstrada::exception("FileWriteLine: Argument is not a file object");
    int fd = fileObj->m_fd;
    if (fd < 0) throw webstrada::exception("FileWriteLine: File is not open");

    string cStr = const_cast<cfvariant*>(content)->toString();
    string data = cStr + "\n";
    write(fd, data.constData(), data.length());
    return cfvariant_create_bool(true);
}

} // namespace cfml

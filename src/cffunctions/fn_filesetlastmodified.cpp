/**
 * @file fn_filesetlastmodified.cpp
 * @brief CFML filesetlastmodified() built-in.
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

cfvariant *cf_filesetlastmodified(const cfvariant *path, const cfvariant *date) {
    if (!path || !date) throw webstrada::exception("FileSetLastModified: Missing argument(s)");
    string pStr = const_cast<cfvariant*>(path)->toString();
    double days = getDaysOrThrow(date, "FileSetLastModified");

    // Convert CF days to time_t (CF epoch: 1899-12-30, Unix epoch: 1970-01-01)
    // Difference between CF epoch and Unix epoch in days: 25569
    time_t t = static_cast<time_t>((days - 25569) * 86400);

    struct timespec ts[2];
    ts[0].tv_sec = t;
    ts[0].tv_nsec = 0;
    ts[1].tv_sec = t;
    ts[1].tv_nsec = 0;

    if (utimensat(AT_FDCWD, pStr.constData(), ts, 0) != 0) {
        throw webstrada::exception("FileSetLastModified: Failed to set time on: " + pStr);
    }
    return cfvariant_create_bool(true);
}

} // namespace cfml

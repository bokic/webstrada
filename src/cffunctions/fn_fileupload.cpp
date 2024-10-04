/**
 * @file fn_fileupload.cpp
 * @brief CFML fileupload() built-in.
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

cfvariant *cf_fileupload(const cfvariant *dest, const cfvariant *fileField,
                         const cfvariant *mimeType, const cfvariant *onConflict,
                         const cfvariant *strict) {
    if (!dest || !fileField) throw webstrada::exception("FileUpload: Missing argument(s)");
    string destStr = const_cast<cfvariant*>(dest)->toString();
    string fieldStr = const_cast<cfvariant*>(fileField)->toString();

    std::string serverDir, fileBase;
    bool dirMode = true;
    resolveUploadDestination(destStr, "FileUpload", serverDir, dirMode, fileBase);

    // Locate the uploaded file by form field name (case-insensitive).
    const auto &files = webstrada::UploadRegistry::instance().files();
    const webstrada::UploadedFile *found = nullptr;
    for (const auto &f : files) {
        if (strcasecmp(f.fieldName.c_str(), fieldStr.constData()) == 0) {
            found = &f;
            break;
        }
    }
    if (!found) {
        throw webstrada::exception("FileUpload: The form field " + fieldStr + " did not contain a file.");
    }

    // MIME type / extension allow-list check.
    string mimeList;
    if (mimeType) mimeList = const_cast<cfvariant*>(mimeType)->toString();
    if (!mimeList.isEmpty() && !mimeTypeMatches(*found, mimeList)) {
        throw webstrada::exception("FileUpload: The uploaded file " + string(found->filename.c_str()) + " is not of an accepted type. Accepted MIME types or extensions: " + mimeList);
    }

    string conflict = "error";
    if (onConflict) conflict = const_cast<cfvariant*>(onConflict)->toString();
    conflict.toLower();

    return saveUploadedFile(*found, serverDir, conflict, fileBase);
}

} // namespace cfml

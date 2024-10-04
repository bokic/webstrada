/**
 * @file fn_filegetmimetype.cpp
 * @brief CFML filegetmimetype() built-in.
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

cfvariant *cf_filegetmimetype(const cfvariant *path) {
    if (!path) throw webstrada::exception("FileGetMimeType: Missing argument");
    string pStr = const_cast<cfvariant*>(path)->toString();

    // Use extension-based MIME type detection
    std::filesystem::path fp(pStr.constData());
    string ext = fp.extension().string().c_str();
    ext.toLower();

    const char *mime = "application/octet-stream";
    if (ext.equals(".txt") || ext.equals(".text")) mime = "text/plain";
    else if (ext.equals(".html") || ext.equals(".htm")) mime = "text/html";
    else if (ext.equals(".css")) mime = "text/css";
    else if (ext.equals(".js")) mime = "application/javascript";
    else if (ext.equals(".json")) mime = "application/json";
    else if (ext.equals(".xml")) mime = "application/xml";
    else if (ext.equals(".pdf")) mime = "application/pdf";
    else if (ext.equals(".zip")) mime = "application/zip";
    else if (ext.equals(".png")) mime = "image/png";
    else if (ext.equals(".jpg") || ext.equals(".jpeg")) mime = "image/jpeg";
    else if (ext.equals(".gif")) mime = "image/gif";
    else if (ext.equals(".svg")) mime = "image/svg+xml";
    else if (ext.equals(".ico")) mime = "image/x-icon";
    else if (ext.equals(".mp3")) mime = "audio/mpeg";
    else if (ext.equals(".mp4")) mime = "video/mp4";
    else if (ext.equals(".avi")) mime = "video/x-msvideo";
    else if (ext.equals(".csv")) mime = "text/csv";
    else if (ext.equals(".doc") || ext.equals(".docx")) mime = "application/msword";
    else if (ext.equals(".xls") || ext.equals(".xlsx")) mime = "application/vnd.ms-excel";

    auto *ret = new cfvariant(mime);
    return ret;
}

} // namespace cfml

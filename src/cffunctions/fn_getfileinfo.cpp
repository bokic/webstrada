/**
 * @file fn_getfileinfo.cpp
 * @brief CFML getfileinfo() built-in.
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

cfvariant *cf_getfileinfo(const cfvariant *path) {
    if (!path) throw webstrada::exception("GetFileInfo: Missing argument");
    string pStr = const_cast<cfvariant*>(path)->toString();
    cfvariant info(cfvariant::Struct);
    bool exists = std::filesystem::exists(pStr.constData());
    info.set("exists").set_type(cfvariant::Boolean);
    info.set("exists").m_bool = exists;
    if (exists) {
        info.set("size").set_type(cfvariant::Number);
        info.set("size").m_int = static_cast<int>(std::filesystem::file_size(pStr.constData()));
        info.set("path").set_type(cfvariant::String);
        info.set("path").m_str->append(std::filesystem::absolute(pStr.constData()).string().c_str());
        info.set("type").set_type(cfvariant::String);
        if (std::filesystem::is_directory(pStr.constData())) {
            info.set("type").m_str->append("directory");
        } else {
            info.set("type").m_str->append("file");
        }
    } else {
        info.set("size").set_type(cfvariant::Number);
        info.set("size").m_int = 0;
        info.set("path").set_type(cfvariant::String);
        info.set("path").m_str->append(pStr);
        info.set("type").set_type(cfvariant::String);
        info.set("type").m_str->append("unknown");
    }
    auto *ret = new cfvariant(info);
    return ret;
}

} // namespace cfml

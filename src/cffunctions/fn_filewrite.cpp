/**
 * @file fn_filewrite.cpp
 * @brief CFML filewrite() built-in.
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

cfvariant *cf_filewrite(const cfvariant *path, const cfvariant *content) {
    if (!path || !content) throw webstrada::exception("FileWrite: Missing argument(s)");
    string pStr = const_cast<cfvariant*>(path)->toString();
    if (content->m_type == cfvariant::Binary && content->m_binary) {
        std::ofstream bout(pStr.constData(), std::ios::binary | std::ios::trunc);
        if (!bout) throw webstrada::exception("FileWrite: Failed to open file for writing: " + pStr);
        bout.write((const char*)content->m_binary->data(), (std::streamsize)content->m_binary->size());
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = true;
        return ret;
    }
    string cStr = const_cast<cfvariant*>(content)->toString();
    // CF's FileWrite(file, string) appends a trailing newline (verified on the
    // RDS host: FileWrite("/tmp/x", "aaaa") yields a 5-byte file), mirroring
    // the <cffile action="write" addnewline="yes"> default. The binary path
    // above writes verbatim.
    std::ofstream outfile(pStr.constData(), std::ios::binary);
    if (!outfile) throw webstrada::exception("FileWrite: Failed to open file for writing: " + pStr);
    outfile << cStr.constData();
    outfile << "\n";
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = true;
    return ret;
}

} // namespace cfml

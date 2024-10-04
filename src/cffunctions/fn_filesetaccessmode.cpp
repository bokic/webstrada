/**
 * @file fn_filesetaccessmode.cpp
 * @brief CFML filesetaccessmode() built-in.
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

cfvariant *cf_filesetaccessmode(const cfvariant *path, const cfvariant *mode) {
    if (!path || !mode) throw webstrada::exception("FileSetAccessMode: Missing argument(s)");
    string pStr = const_cast<cfvariant*>(path)->toString();
    string mStr = const_cast<cfvariant*>(mode)->toString();

    mode_t perm = 0;
    if (mStr.length() == 3) {
        for (int i = 0; i < 3; i++) {
            int digit = mStr.at(i) - '0';
            if (digit < 0 || digit > 7) {
                throw webstrada::exception("FileSetAccessMode: Invalid mode '" + mStr + "'. Use numeric octal (e.g., 644)");
            }
            perm = (perm << 3) | digit;
        }
    } else if (mStr.length() == 4) {
        // Handle leading special bits
        for (int i = 0; i < 4; i++) {
            int digit = mStr.at(i) - '0';
            if (digit < 0 || digit > 7) {
                throw webstrada::exception("FileSetAccessMode: Invalid mode '" + mStr + "'. Use numeric octal (e.g., 644)");
            }
            perm = (perm << 3) | digit;
        }
    } else {
        throw webstrada::exception("FileSetAccessMode: Invalid mode '" + mStr + "'. Use numeric octal (e.g., 644)");
    }

    if (chmod(pStr.constData(), perm) != 0) {
        throw webstrada::exception("FileSetAccessMode: Failed to set mode on: " + pStr);
    }
    return cfvariant_create_bool(true);
}

} // namespace cfml

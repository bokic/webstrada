/**
 * @file fn_isimagefile.cpp
 * @brief CFML isimagefile() built-in.
 */

#include "common.h"

#include <webstrada/cfimage.h>
#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <cairo.h>
#include <jpeglib.h>
#include <zlib.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <setjmp.h>
#include <fstream>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

using webstrada::cfvariant;
using webstrada::ImageData;
using webstrada::string;
using webstrada::exception;
using webstrada::cfvariant;
using webstrada::ImageData;
using webstrada::string;
using webstrada::exception;

namespace cfml {

cfvariant *cf_isimagefile(const cfvariant *value, const cfvariant *format)
{
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    std::string path = toStdString(value);
    std::string wantFmt = format ? toLower(toStdString(format)) : "";
    if (!path.empty()) {
        try {
            std::vector<std::byte> bytes = readFileBytes(path);
            std::string fmt = sniffFormat(bytes);
            if (!fmt.empty() && (wantFmt.empty() || fmt == wantFmt || fmt == (wantFmt == "jpg" ? "jpeg" : wantFmt))) {
                ImageData *img = imageFromBytes(bytes, fmt, path);
                image_data_release(img);
                ret->m_bool = true;
            }
        } catch (...) {
            // not a decodable image -> false
        }
    }
    return ret;
}

} // namespace cfml

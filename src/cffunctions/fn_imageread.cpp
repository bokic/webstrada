/**
 * @file fn_imageread.cpp
 * @brief CFML imageread() built-in.
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

cfvariant *cf_imageread(const cfvariant *path)
{
    if (!path) throw exception("ImageRead requires exactly 1 argument");
    std::string p = toStdString(path);
    if (p.empty()) throw exception("The system cannot find the file specified.");
    std::vector<std::byte> bytes = readFileBytes(p);
    std::string fmt = sniffFormat(bytes);
    if (fmt.empty()) throw exception("The file is not a valid image file.");
    ImageData *img = imageFromBytes(bytes, fmt, resolveSourcePath(p));
    auto *ret = new cfvariant(cfvariant::Image);
    ret->m_image = img;
    return ret;
}

} // namespace cfml

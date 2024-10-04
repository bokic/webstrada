/**
 * @file fn_imagewrite.cpp
 * @brief CFML imagewrite() built-in.
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

cfvariant *cf_imagewrite(const cfvariant *image, const cfvariant *destination,
                         const cfvariant *quality, const cfvariant *overwrite)
{
    ImageData *img = image_from_variant(image);
    std::string dest = toStdString(destination);
    std::string fmt = fileExt(dest);
    if (fmt.empty()) {
        imageThrow("Application", "The destination file should contain an extension, so that ColdFusion can determine the image format.",
                   "Verify your inputs. The destination file should contain an extension, so that ColdFusion can determine the image format.");
    }
    double q = 0.75;
    if (quality && !toStdString(quality).empty() && quality->m_type != cfvariant::Null) q = toDouble(quality);
    if ((fmt == "jpeg" || fmt == "jpg" || fmt == "jfif") && (q < 0.0 || q > 1.0)) {
        imageThrow("Application", "The JPEG Quality value should be between 0 and 1.",
                   "Verify your inputs. The JPEG Quality value should be between 0 and 1.");
    }
    bool ow = overwrite ? toBool(overwrite) : true;
    std::vector<std::byte> bytes = encodeImage(img, fmt, q);
    writeFileBytes(dest, bytes, ow);
    auto *ret = new cfvariant(cfvariant::Null);
    return ret;
}

} // namespace cfml

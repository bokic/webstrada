/**
 * @file fn_imagegetblob.cpp
 * @brief CFML imagegetblob() built-in.
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

cfvariant *cf_imagegetblob(const cfvariant *image)
{
    ImageData *img = image_from_variant(image);
    if (img->sourceFormat.empty()) {
        imageThrow("Application", "The source file should contain an extension,so that ColdFusion can determine the image format.",
                   "Verify your inputs. The source file should contain an extension,so that ColdFusion can determine the image format.");
    }
    double q = 0.75;
    std::vector<std::byte> bytes = encodeImage(img, img->sourceFormat, q);
    auto *ret = new cfvariant(cfvariant::Binary);
    *ret->m_binary = std::move(bytes);
    return ret;
}

} // namespace cfml

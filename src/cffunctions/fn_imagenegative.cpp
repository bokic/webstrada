/**
 * @file fn_imagenegative.cpp
 * @brief CFML imagenegative() built-in.
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

cfvariant *cf_imagenegative(const cfvariant *image)
{
    ImageData *img = image_from_variant(image);
    imageTransform(img, [](uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) {
        r = 255 - r; g = 255 - g; b = 255 - b;
    });
    return nullResult();
}

} // namespace cfml

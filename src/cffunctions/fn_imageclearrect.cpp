/**
 * @file fn_imageclearrect.cpp
 * @brief CFML imageclearrect() built-in.
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

cfvariant *cf_imageclearrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                             const cfvariant *width, const cfvariant *height)
{
    ImageData *img = image_from_variant(image);
    int X = toInt(x), Y = toInt(y), W = toInt(width), H = toInt(height);
    fillRectColor(img, X, Y, W, H, img->backgroundColor);
    return nullResult();
}

} // namespace cfml

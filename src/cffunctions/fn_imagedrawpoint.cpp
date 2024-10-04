/**
 * @file fn_imagedrawpoint.cpp
 * @brief CFML imagedrawpoint() built-in.
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

cfvariant *cf_imagedrawpoint(const cfvariant *image, const cfvariant *x, const cfvariant *y)
{
    ImageData *img = image_from_variant(image);
    int X = toInt(x), Y = toInt(y);
    int w = (int)img->strokeWidth;
    if (w < 1) w = 1;
    int off = (w - 1) / 2;
    // Java2D point: a (w+1) x w block anchored at (X - off, Y - off).
    paintShape(img, [&](cairo_t *cr) {
        cairo_rectangle(cr, X - off, Y - off, w + 1, w);
        cairo_fill(cr);
    });
    return nullResult();
}

} // namespace cfml

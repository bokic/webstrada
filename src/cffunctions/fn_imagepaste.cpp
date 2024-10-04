/**
 * @file fn_imagepaste.cpp
 * @brief CFML imagepaste() built-in.
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

cfvariant *cf_imagepaste(const cfvariant *image1, const cfvariant *image2,
                         const cfvariant *x, const cfvariant *y)
{
    ImageData *img1 = image_from_variant(image1);
    ImageData *img2 = image_from_variant(image2);
    int X = toInt(x), Y = toInt(y);
    // CF draws image2 over image1 with the top-left at (x,y), bicubic.
    cairo_t *cr = cairo_create(img1->surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, img2->surface, X, Y);
    cairo_paint(cr);
    cairo_destroy(cr);
    return nullResult();
}

} // namespace cfml

/**
 * @file fn_imagesheardrawingaxis.cpp
 * @brief CFML imagesheardrawingaxis() built-in.
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

static void drawingShear(ImageData *img, double shrx, double shry)
{
    cairo_matrix_t t;
    t.xx = 1; t.yx = shry; t.xy = shrx; t.yy = 1; t.x0 = 0; t.y0 = 0;
    drawingPostMul(img, t);
}

cfvariant *cf_imagesheardrawingaxis(const cfvariant *image, const cfvariant *shrx, const cfvariant *shry)
{
    ImageData *img = image_from_variant(image);
    drawingShear(img, toDouble(shrx), toDouble(shry));
    return nullResult();
}

} // namespace cfml

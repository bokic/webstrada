/**
 * @file fn_imagetranslatedrawingaxis.cpp
 * @brief CFML imagetranslatedrawingaxis() built-in.
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

static void drawingTranslate(ImageData *img, double tx, double ty)
{
    cairo_matrix_t t;
    cairo_matrix_init_translate(&t, tx, ty);
    drawingPostMul(img, t);
}

cfvariant *cf_imagetranslatedrawingaxis(const cfvariant *image, const cfvariant *x, const cfvariant *y)
{
    ImageData *img = image_from_variant(image);
    drawingTranslate(img, toDouble(x), toDouble(y));
    return nullResult();
}

} // namespace cfml

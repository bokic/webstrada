/**
 * @file fn_imagerotatedrawingaxis.cpp
 * @brief CFML imagerotatedrawingaxis() built-in.
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

static void drawingRotate(ImageData *img, double angle, double x, double y)
{
    double a = angle * M_PI / 180.0;
    double c = cos(a), s = sin(a);
    // A = translate(x,y) * R * translate(-x,-y)
    cairo_matrix_t t;
    t.xx = c;  t.yx = s;  t.xy = -s;  t.yy = c;
    t.x0 = x - (c * x - s * y);
    t.y0 = y - (s * x + c * y);
    drawingPostMul(img, t);
}

cfvariant *cf_imagerotatedrawingaxis(const cfvariant *image, const cfvariant *angle, const cfvariant *x,
                                     const cfvariant *y)
{
    ImageData *img = image_from_variant(image);
    double px = (x && x->m_type != cfvariant::Null) ? toDouble(x) : 0.0;
    double py = (y && y->m_type != cfvariant::Null) ? toDouble(y) : 0.0;
    drawingRotate(img, toDouble(angle), px, py);
    return nullResult();
}

} // namespace cfml

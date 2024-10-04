/**
 * @file fn_imagedrawoval.cpp
 * @brief CFML imagedrawoval() built-in.
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

static void ellipsePath(cairo_t *cr, double x, double y, double w, double h)
{
    double cx = x + w / 2.0, cy = y + h / 2.0;
    double rx = w / 2.0, ry = h / 2.0;
    const int N = 256;
    cairo_move_to(cr, cx + rx, cy);
    for (int i = 1; i < N; i++) {
        double t = 2.0 * M_PI * i / N;
        cairo_line_to(cr, cx + rx * cos(t), cy + ry * sin(t));
    }
    cairo_close_path(cr);
}

cfvariant *cf_imagedrawoval(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                            const cfvariant *width, const cfvariant *height, const cfvariant *filled)
{
    ImageData *img = image_from_variant(image);
    double X = toDouble(x), Y = toDouble(y), W = toDouble(width), H = toDouble(height);
    bool fill = toBool(filled);
    paintShape(img, [&](cairo_t *cr) {
        ellipsePath(cr, X, Y, W, H);
        if (fill) cairo_fill(cr); else cairo_stroke(cr);
    });
    return nullResult();
}

} // namespace cfml

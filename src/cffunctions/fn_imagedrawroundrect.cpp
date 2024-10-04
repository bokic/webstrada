/**
 * @file fn_imagedrawroundrect.cpp
 * @brief CFML imagedrawroundrect() built-in.
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

static void roundedRectPath(cairo_t *cr, double x, double y, double w, double h,
                            double arcw, double archeight)
{
    double rx = std::min(arcw / 2.0, w / 2.0);
    double ry = std::min(archeight / 2.0, h / 2.0);
    if (rx < 0) rx = 0;
    if (ry < 0) ry = 0;
    if (rx == 0 || ry == 0) {
        cairo_rectangle(cr, x, y, w, h);
        return;
    }
    const double x0 = x, y0 = y, x1 = x + w, y1 = y + h;
    auto corner = [&](double cx, double cy, double startDeg, double sweepDeg) {
        const int steps = 32;
        for (int i = 0; i <= steps; i++) {
            double t = (startDeg + sweepDeg * i / steps) * M_PI / 180.0;
            cairo_line_to(cr, cx + rx * cos(t), cy + ry * sin(t));
        }
    };
    cairo_move_to(cr, x0 + rx, y0);
    cairo_line_to(cr, x1 - rx, y0);
    corner(x1 - rx, y0 + ry, -90, 90);
    cairo_line_to(cr, x1, y1 - ry);
    corner(x1 - rx, y1 - ry, 0, 90);
    cairo_line_to(cr, x0 + rx, y1);
    corner(x0 + rx, y1 - ry, 90, 90);
    cairo_line_to(cr, x0, y0 + ry);
    corner(x0 + rx, y0 + ry, 180, 90);
    cairo_close_path(cr);
}

cfvariant *cf_imagedrawroundrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                                 const cfvariant *width, const cfvariant *height, const cfvariant *arcwidth,
                                 const cfvariant *archeight, const cfvariant *filled)
{
    ImageData *img = image_from_variant(image);
    double X = toDouble(x), Y = toDouble(y), W = toDouble(width), H = toDouble(height);
    double aw = toDouble(arcwidth), ah = toDouble(archeight);
    bool fill = toBool(filled);
    paintShape(img, [&](cairo_t *cr) {
        roundedRectPath(cr, X, Y, W, H, aw, ah);
        if (fill) cairo_fill(cr); else cairo_stroke(cr);
    });
    return nullResult();
}

} // namespace cfml

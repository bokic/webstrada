/**
 * @file fn_imagedrawarc.cpp
 * @brief CFML imagedrawarc() built-in.
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

static void arcPath(cairo_t *cr, double x, double y, double w, double h,
                    double startAngle, double arcAngle, bool pie)
{
    double cx = x + w / 2.0, cy = y + h / 2.0;
    double rx = w / 2.0, ry = h / 2.0;
    auto pt = [&](double javaDeg) {
        double rad = javaDeg * M_PI / 180.0;
        return std::make_pair(cx + rx * cos(rad), cy - ry * sin(rad));
    };
    auto p0 = pt(startAngle);
    if (pie) {
        cairo_move_to(cr, cx, cy);
        cairo_line_to(cr, p0.first, p0.second);
    } else {
        cairo_move_to(cr, p0.first, p0.second);
    }
    int steps = (int)std::ceil(std::abs(arcAngle));
    if (steps < 1) steps = 1;
    for (int i = 1; i <= steps; i++) {
        auto p = pt(startAngle + arcAngle * i / steps);
        cairo_line_to(cr, p.first, p.second);
    }
    if (pie) cairo_close_path(cr);
}

cfvariant *cf_imagedrawarc(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                           const cfvariant *width, const cfvariant *height, const cfvariant *startAngle,
                           const cfvariant *arcAngle, const cfvariant *filled)
{
    ImageData *img = image_from_variant(image);
    double X = toDouble(x), Y = toDouble(y), W = toDouble(width), H = toDouble(height);
    double start = toDouble(startAngle), arc = toDouble(arcAngle);
    bool fill = toBool(filled);
    paintShape(img, [&](cairo_t *cr) {
        arcPath(cr, X, Y, W, H, start, arc, fill);
        if (fill) cairo_fill(cr); else cairo_stroke(cr);
    });
    return nullResult();
}

} // namespace cfml

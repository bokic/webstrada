/**
 * @file fn_imagedrawquadraticcurve.cpp
 * @brief CFML imagedrawquadraticcurve() built-in.
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

cfvariant *cf_imagedrawquadraticcurve(const cfvariant *image, const cfvariant *x1, const cfvariant *y1,
                                      const cfvariant *ctrlx1, const cfvariant *ctrly1, const cfvariant *x2,
                                      const cfvariant *y2)
{
    ImageData *img = image_from_variant(image);
    double sx = toDouble(x1), sy = toDouble(y1);
    double cx = toDouble(ctrlx1), cy = toDouble(ctrly1);
    double ex = toDouble(x2), ey = toDouble(y2);
    // Convert quadratic (start, control, end) to a cubic Bezier.
    double c1x = sx + 2.0 / 3.0 * (cx - sx), c1y = sy + 2.0 / 3.0 * (cy - sy);
    double c2x = ex + 2.0 / 3.0 * (cx - ex), c2y = ey + 2.0 / 3.0 * (cy - ey);
    paintShape(img, [&](cairo_t *cr) {
        cairo_move_to(cr, sx, sy);
        cairo_curve_to(cr, c1x, c1y, c2x, c2y, ex, ey);
        cairo_stroke(cr);
    });
    return nullResult();
}

} // namespace cfml

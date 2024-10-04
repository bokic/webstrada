/**
 * @file fn_imagedrawcubiccurve.cpp
 * @brief CFML imagedrawcubiccurve() built-in.
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

cfvariant *cf_imagedrawcubiccurve(const cfvariant *image, const cfvariant *ctrlx1, const cfvariant *ctrly1,
                                  const cfvariant *ctrlx2, const cfvariant *ctrly2, const cfvariant *x1,
                                  const cfvariant *y1, const cfvariant *x2, const cfvariant *y2)
{
    ImageData *img = image_from_variant(image);
    double sx = toDouble(ctrlx1), sy = toDouble(ctrly1);
    double c1x = toDouble(ctrlx2), c1y = toDouble(ctrly2);
    double c2x = toDouble(x1), c2y = toDouble(y1);
    double ex = toDouble(x2), ey = toDouble(y2);
    paintShape(img, [&](cairo_t *cr) {
        cairo_move_to(cr, sx, sy);
        cairo_curve_to(cr, c1x, c1y, c2x, c2y, ex, ey);
        cairo_stroke(cr);
    });
    return nullResult();
}

} // namespace cfml

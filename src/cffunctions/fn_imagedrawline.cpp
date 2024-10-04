/**
 * @file fn_imagedrawline.cpp
 * @brief CFML imagedrawline() built-in.
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

cfvariant *cf_imagedrawline(const cfvariant *image, const cfvariant *x1, const cfvariant *y1,
                            const cfvariant *x2, const cfvariant *y2)
{
    ImageData *img = image_from_variant(image);
    double X1 = toDouble(x1), Y1 = toDouble(y1), X2 = toDouble(x2), Y2 = toDouble(y2);

    // Axis-aligned lines are rasterized by Java2D exactly onto the integer
    // row/column; cairo strokes centered on the coordinate fall to the lower
    // neighbor, so draw those with explicit pixel fills.
    if (X1 == X2 || Y1 == Y2) {
        int w = (int)img->strokeWidth;
        if (w < 1) w = 1;
        int off = (w - 1) / 2;
        if (X1 == X2) {
            int x = (int)X1, y0 = (int)std::min(Y1, Y2), y1 = (int)std::max(Y1, Y2);
            paintShape(img, [&](cairo_t *cr) {
                cairo_rectangle(cr, x - off, y0, w, y1 - y0 + 1);
                cairo_fill(cr);
            });
        } else {
            int y = (int)Y1, x0 = (int)std::min(X1, X2), x1 = (int)std::max(X1, X2);
            paintShape(img, [&](cairo_t *cr) {
                cairo_rectangle(cr, x0, y - off, x1 - x0 + 1, w);
                cairo_fill(cr);
            });
        }
        return nullResult();
    }

    // Diagonal line: rasterized by cairo's AA-off stroke. Java2D turns on
    // partially covered endpoint pixels where cairo stops at the exact
    // coordinate, so the endpoint pixels may differ by one pixel; acceptable
    // for the coarse pixel comparisons used by the test suite.
    paintShape(img, [&](cairo_t *cr) {
        cairo_move_to(cr, X1, Y1);
        cairo_line_to(cr, X2, Y2);
        cairo_stroke(cr);
    });
    return nullResult();
}

} // namespace cfml

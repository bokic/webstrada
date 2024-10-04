/**
 * @file fn_imagedrawrect.cpp
 * @brief CFML imagedrawrect() built-in.
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

cfvariant *cf_imagedrawrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                            const cfvariant *width, const cfvariant *height, const cfvariant *filled)
{
    ImageData *img = image_from_variant(image);
    int X = toInt(x), Y = toInt(y), W = toInt(width), H = toInt(height);
    bool fill = toBool(filled);
    paintShape(img, [&](cairo_t *cr) {
        if (fill) {
            cairo_rectangle(cr, X, Y, W, H);
            cairo_fill(cr);
        } else {
            // Java drawRect outline spans [X, X+W] x [Y, Y+H].
            cairo_rectangle(cr, X, Y, W + 1, 1);
            cairo_rectangle(cr, X, Y + H, W + 1, 1);
            cairo_rectangle(cr, X, Y, 1, H + 1);
            cairo_rectangle(cr, X + W, Y, 1, H + 1);
            cairo_fill(cr);
        }
    });
    return nullResult();
}

} // namespace cfml

/**
 * @file fn_imagecrop.cpp
 * @brief CFML imagecrop() built-in.
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

cfvariant *cf_imagecrop(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                        const cfvariant *width, const cfvariant *height)
{
    ImageData *img = image_from_variant(image);
    int X = toInt(x), Y = toInt(y);
    int W = toInt(width), H = toInt(height);
    if (W < 0 || H < 0) {
        imageThrow("Application", "Height and width parameters should be non-negative.",
                   "Verify your inputs. Height and width parameters should be non-negative.");
    }
    // CF clamps the crop rect to the image bounds.
    if (img->width < X + W) W = img->width - X;
    if (img->height < Y + H) H = img->height - Y;
    if (W <= 0 || H <= 0) {
        // Empty crop: CF produces an empty image.
        cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
        imageReplaceSurface(img, out, 0, 0);
        return nullResult();
    }
    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
    cairo_t *cr = cairo_create(out);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, img->surface, -X, -Y);
    cairo_paint(cr);
    cairo_destroy(cr);
    imageReplaceSurface(img, out, W, H);
    return nullResult();
}

} // namespace cfml

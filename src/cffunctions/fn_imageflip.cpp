/**
 * @file fn_imageflip.cpp
 * @brief CFML imageflip() built-in.
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

cfvariant *cf_imageflip(const cfvariant *image, const cfvariant *transpose)
{
    ImageData *img = image_from_variant(image);
    std::string t = transpose ? toLower(toStdString(transpose)) : "vertical";
    if (t != "vertical" && t != "horizontal" && t != "diagonal" && t != "antidiagonal" &&
        t != "90" && t != "180" && t != "270") {
        imageThrow("Application", "The transpose argument must be  vertical, horizontal, diagonal, antidiagonal, 90, 180, or 270.",
                   "Verify your inputs. The transpose argument must be  vertical, horizontal, diagonal, antidiagonal, 90, 180, or 270.");
    }

    int w = img->width, h = img->height;
    uint8_t r, g, b, a;

    // vertical: rows reversed; horizontal: columns reversed; 180: both.
    // These keep the image dimensions.
    if (t == "vertical" || t == "horizontal" || t == "180") {
        cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        cairo_t *cr = cairo_create(out);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                imgPixel(img, x, y, &r, &g, &b, &a);
                int dx = x, dy = y;
                if (t == "vertical")        { dx = x;      dy = h - 1 - y; }
                else if (t == "horizontal") { dx = w - 1 - x; dy = y; }
                else                        { dx = w - 1 - x; dy = h - 1 - y; }
                surfaceSetRGBA(out, dx, dy, r, g, b, a);
            }
        }
        imageReplaceSurface(img, out, w, h);
        return nullResult();
    }

    if (t == "diagonal") {
        // Swap axes: dest(x',y') = src(y', x'). Output is (h, w).
        cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, h, w);
        cairo_t *cr = cairo_create(out);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                imgPixel(img, x, y, &r, &g, &b, &a);
                surfaceSetRGBA(out, y, x, r, g, b, a);
            }
        imageReplaceSurface(img, out, h, w);
        return nullResult();
    }

    if (t == "antidiagonal") {
        // Flip across the anti-diagonal (upper-right to lower-left): output is
        // the transpose with both axes reversed. dest(x',y') = src(h-1-y', w-1-x').
        cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, h, w);
        cairo_t *cr = cairo_create(out);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                imgPixel(img, x, y, &r, &g, &b, &a);
                surfaceSetRGBA(out, h - 1 - y, w - 1 - x, r, g, b, a);
            }
        imageReplaceSurface(img, out, h, w);
        return nullResult();
    }

    if (t == "90") {
        // Clockwise 90: src(x,y) -> dest(h-1-y, x). Output is (h, w).
        cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, h, w);
        cairo_t *cr = cairo_create(out);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                imgPixel(img, x, y, &r, &g, &b, &a);
                surfaceSetRGBA(out, h - 1 - y, x, r, g, b, a);
            }
        imageReplaceSurface(img, out, h, w);
        return nullResult();
    }

    // t == "270": clockwise 270 = counter-clockwise 90: src(x,y) -> dest(y, w-1-x).
    {
        cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, h, w);
        cairo_t *cr = cairo_create(out);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                imgPixel(img, x, y, &r, &g, &b, &a);
                surfaceSetRGBA(out, y, w - 1 - x, r, g, b, a);
            }
        imageReplaceSurface(img, out, h, w);
        return nullResult();
    }
    return nullResult();
}

} // namespace cfml

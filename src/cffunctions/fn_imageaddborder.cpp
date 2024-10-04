/**
 * @file fn_imageaddborder.cpp
 * @brief CFML imageaddborder() built-in.
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

cfvariant *cf_imageaddborder(const cfvariant *image, const cfvariant *thickness,
                             const cfvariant *color, const cfvariant *borderType)
{
    ImageData *img = image_from_variant(image);
    int t = toInt(thickness);
    if (t < 0) {
        imageThrow("Application", "Border width parameters should be non-negative.",
                   "Verify your inputs. Border width parameters should be non-negative.");
    }
    std::string type = (borderType && borderType->m_type != cfvariant::Null) ? toLower(toStdString(borderType)) : "constant";
    if (type != "zero" && type != "copy" && type != "reflect" && type != "wrap" && type != "constant") {
        imageThrow("Application", "Border type should be zero, copy, constant, reflect, or wrap.",
                   "Verify your inputs. Border type should be zero, copy, constant, reflect, or wrap.");
    }
    uint32_t borderColor = 0xFF000000u; // zero/constant default black
    if (type == "constant") borderColor = imgParseColor(color);

    int w = img->width, h = img->height;
    int outW = w + 2 * t, outH = h + 2 * t;
    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, outW, outH);
    {
        cairo_t *cr = cairo_create(out);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        if (type == "zero") {
            cairo_set_source_rgba(cr, 0, 0, 0, 1);
            cairo_paint(cr);
        } else if (type == "constant") {
            double a = ((borderColor >> 24) & 0xFF) / 255.0;
            cairo_set_source_rgba(cr, ((borderColor>>16)&0xFF)/255.0, ((borderColor>>8)&0xFF)/255.0,
                                  (borderColor&0xFF)/255.0, a);
            cairo_paint(cr);
        }
        // copy/reflect/wrap leave the border transparent initially; filled below.
        cairo_destroy(cr);
    }
    // Copy the inner image.
    cairo_t *cr = cairo_create(out);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, img->surface, t, t);
    cairo_paint(cr);
    cairo_destroy(cr);

    if (type == "copy" || type == "reflect" || type == "wrap") {
        // Fill the 4 border bands by sampling the edge.
        uint8_t r, g, b, a;
        auto edge = [&](int sx, int sy, int *dx, int *dy) {
            // caller-provided mapping
        };
        (void)edge;
        // top band
        for (int by = 0; by < t; by++)
            for (int bx = 0; bx < outW; bx++) {
                int sx = bx - t, sy;
                if (type == "copy")      sy = 0;
                else if (type == "wrap") sy = (h - 1) - ((t - 1 - by) % h);
                else { // reflect
                    int row = (t - 1 - by) % (2 * h);
                    sy = row < h ? row : 2 * h - 1 - row;
                }
                if (sx < 0 || sx >= w) continue;
                imgPixel(img, sx, sy, &r, &g, &b, &a);
                surfaceSetRGBA(out, bx, by, r, g, b, a);
            }
        // bottom band
        for (int by = 0; by < t; by++)
            for (int bx = 0; bx < outW; bx++) {
                int sx = bx - t, sy;
                if (type == "copy")      sy = h - 1;
                else if (type == "wrap") sy = by % h;
                else {
                    int row = by % (2 * h);
                    sy = row < h ? row : 2 * h - 1 - row;
                }
                if (sx < 0 || sx >= w) continue;
                imgPixel(img, sx, sy, &r, &g, &b, &a);
                surfaceSetRGBA(out, bx, t + h + by, r, g, b, a);
            }
        // left band
        for (int bx = 0; bx < t; bx++)
            for (int by = 0; by < h; by++) {
                int sx, sy = by;
                if (type == "copy")      sx = 0;
                else if (type == "wrap") sx = (w - 1) - ((t - 1 - bx) % w);
                else {
                    int col = (t - 1 - bx) % (2 * w);
                    sx = col < w ? col : 2 * w - 1 - col;
                }
                imgPixel(img, sx, sy, &r, &g, &b, &a);
                surfaceSetRGBA(out, bx, t + by, r, g, b, a);
            }
        // right band
        for (int bx = 0; bx < t; bx++)
            for (int by = 0; by < h; by++) {
                int sx, sy = by;
                if (type == "copy")      sx = w - 1;
                else if (type == "wrap") sx = bx % w;
                else {
                    int col = bx % (2 * w);
                    sx = col < w ? col : 2 * w - 1 - col;
                }
                imgPixel(img, sx, sy, &r, &g, &b, &a);
                surfaceSetRGBA(out, t + w + bx, t + by, r, g, b, a);
            }
    }
    imageReplaceSurface(img, out, outW, outH);
    return nullResult();
}

} // namespace cfml

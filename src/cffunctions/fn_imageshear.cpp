/**
 * @file fn_imageshear.cpp
 * @brief CFML imageshear() built-in.
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

cfvariant *cf_imageshear(const cfvariant *image, const cfvariant *shear,
                         const cfvariant *direction, const cfvariant *interpolation)
{
    ImageData *img = image_from_variant(image);
    double s = toDouble(shear);
    std::string dir = (direction && direction->m_type != cfvariant::Null) ? toLower(toStdString(direction)) : "horizontal";
    if (dir != "horizontal" && dir != "vertical") {
        imageThrow("Application", "The shear direction must be Horizontal or Vertical.",
                   "Verify your inputs. The shear direction must be Horizontal or Vertical.");
    }
    if (interpolation && interpolation->m_type != cfvariant::Null) {
        std::string interp = toLower(toStdString(interpolation));
        if (interp != "nearest" && interp != "bilinear" && interp != "bicubic") {
            imageThrow("Application", "The interpolation argument must be one of: NEAREST|BILINEAR|BICUBIC",
                       "Verify your inputs. The interpolation argument must be one of: NEAREST|BILINEAR|BICUBIC");
        }
    }
    // Shear about the origin (CF/JAI passes origin 0,0): horizontal shear maps
    // x' = x + s*y, y' = y; vertical maps x' = x, y' = y + s*x. Output size is
    // floor(w + |s|*h) for horizontal, floor(h + |s|*w) for vertical (verified
    // against CF: 8x5 with s=0.5 -> 10x5, s=1.0 -> 13x5, s=1.5 -> 15x5).
    // Background = image background color (default black).
    int w = img->width, h = img->height;
    int outW = (dir == "horizontal") ? (int)floor(w + fabs(s) * h + 1e-9) : w;
    int outH = (dir == "vertical") ? (int)floor(h + fabs(s) * w + 1e-9) : h;
    if (outW < 1) outW = 1;
    if (outH < 1) outH = 1;
    double transX = (dir == "horizontal") ? ((s > 0) ? 0.0 : fabs(s) * h) : 0.0;
    double transY = (dir == "vertical") ? ((s > 0) ? 0.0 : fabs(s) * w) : 0.0;

    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, outW, outH);
    {
        uint32_t bg = img->backgroundColor;
        cairo_t *cr = cairo_create(out);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, ((bg>>16)&0xFF)/255.0, ((bg>>8)&0xFF)/255.0, (bg&0xFF)/255.0, 1.0);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
    bool bilinear = (interpolation && interpolation->m_type != cfvariant::Null &&
                     toLower(toStdString(interpolation)) != "nearest");
    for (int dy = 0; dy < outH; dy++) {
        for (int dx = 0; dx < outW; dx++) {
            double ox = dx - transX, oy = dy - transY;
            double sx, sy;
            // Inverse of the shear map above.
            if (dir == "horizontal") { sx = ox - s * oy; sy = oy; }
            else                     { sx = ox;          sy = oy - s * ox; }
            if (sx < 0 || sy < 0 || sx >= w || sy >= h) continue;
            uint8_t r, g, b, a;
            samplePixel(img, sx, sy, bilinear, &r, &g, &b, &a);
            surfaceSetRGBA(out, dx, dy, r, g, b, a);
        }
    }
    imageReplaceSurface(img, out, outW, outH);
    return nullResult();
}

} // namespace cfml

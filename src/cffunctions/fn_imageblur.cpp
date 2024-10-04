/**
 * @file fn_imageblur.cpp
 * @brief CFML imageblur() built-in.
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

cfvariant *cf_imageblur(const cfvariant *image, const cfvariant *blurRadius)
{
    ImageData *img = image_from_variant(image);
    int radius = (blurRadius && blurRadius->m_type != cfvariant::Null) ? toInt(blurRadius) : 3;
    if (radius < 3 || radius > 10) {
        imageThrow("Application", "The blurRadius must be between 3 and 10 (3 <= radius <= 10).",
                   "Verify your inputs. The blurRadius must be between 3 and 10 (3 <= radius <= 10).");
    }
    // Box filter of width (2*radius+1) with border extender (copy).
    int w = img->width, h = img->height;
    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    const int k = radius;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            long long sr = 0, sg = 0, sb = 0, sa = 0;
            int n = 0;
            for (int dy = -k; dy <= k; dy++) {
                for (int dx = -k; dx <= k; dx++) {
                    int sx = x + dx, sy = y + dy;
                    if (sx < 0) sx = 0; if (sx >= w) sx = w - 1;
                    if (sy < 0) sy = 0; if (sy >= h) sy = h - 1;
                    uint8_t r, g, b, a;
                    imgPixel(img, sx, sy, &r, &g, &b, &a);
                    sr += r; sg += g; sb += b; sa += a;
                    n++;
                }
            }
            surfaceSetRGBA(out, x, y, (uint8_t)(sr / n), (uint8_t)(sg / n), (uint8_t)(sb / n), (uint8_t)(sa / n));
        }
    }
    imageReplaceSurface(img, out, w, h);
    return nullResult();
}

} // namespace cfml

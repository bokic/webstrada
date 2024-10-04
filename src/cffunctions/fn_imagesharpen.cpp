/**
 * @file fn_imagesharpen.cpp
 * @brief CFML imagesharpen() built-in.
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

cfvariant *cf_imagesharpen(const cfvariant *image, const cfvariant *gain)
{
    ImageData *img = image_from_variant(image);
    double g = (gain && gain->m_type != cfvariant::Null) ? toDouble(gain) : 1.0;
    if (g < -1.0f || g > 2.0f) {
        imageThrow("Application", "Gain value is out of range. Its range is between -1 and 2, including fractional values.",
                   "Verify your inputs. Gain value is out of range. Its range is between -1 and 2, including fractional values.");
    }
    // Unsharp mask: sharpened = original + gain * (original - blurred).
    // Blur with a 3x3 box.
    int w = img->width, h = img->height;
    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t r, g, b, a;
            imgPixel(img, x, y, &r, &g, &b, &a);
            long long br = 0, bg = 0, bb = 0;
            int n = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int sx = x + dx, sy = y + dy;
                    if (sx < 0) sx = 0; if (sx >= w) sx = w - 1;
                    if (sy < 0) sy = 0; if (sy >= h) sy = h - 1;
                    uint8_t cr, cg, cb, ca;
                    imgPixel(img, sx, sy, &cr, &cg, &cb, &ca);
                    br += cr; bg += cg; bb += cb;
                    n++;
                }
            auto sharp = [&](int orig, long long blurSum)->uint8_t {
                double blur = (double)blurSum / n;
                double v = orig + g * (orig - blur);
                if (v < 0) v = 0;
                if (v > 255) v = 255;
                return (uint8_t)(v + 0.5);
            };
            surfaceSetRGBA(out, x, y, sharp(r, br), sharp(g, bg), sharp(b, bb), a);
        }
    }
    imageReplaceSurface(img, out, w, h);
    return nullResult();
}

} // namespace cfml

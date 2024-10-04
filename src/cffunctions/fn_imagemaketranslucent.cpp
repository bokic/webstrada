/**
 * @file fn_imagemaketranslucent.cpp
 * @brief CFML imagemaketranslucent() built-in.
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

cfvariant *cf_imagemaketranslucent(const cfvariant *image, const cfvariant *percent)
{
    ImageData *src = image_from_variant(image);
    double p = toDouble(percent);
    if (p < 0.0 || p > 100.0) {
        imageThrow("Application", "The transparency value is a percentage. It should be between 0 and 100.",
                   "Verify your inputs. The transparency value is a percentage. It should be between 0 and 100.");
    }
    // CF: alpha = 1 - percent/100 applied to every pixel (opaque->alpha).
    ImageData *dst = imageClone(src);
    double alpha = 1.0 - p / 100.0;
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            uint8_t r, g, b, a;
            imgPixel(dst, x, y, &r, &g, &b, &a);
            uint8_t na = (uint8_t)((a * alpha) + 0.5);
            if (a == 255 && alpha < 1.0) na = (uint8_t)(alpha * 255.0 + 0.5);
            surfaceSetRGBA(dst->surface, x, y, r, g, b, na);
        }
    }
    dst->colormodel = "argb";
    dst->colormodelType = "PackedColorModel";
    return imageResult(dst);
}

} // namespace cfml

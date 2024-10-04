/**
 * @file fn_imagemakecolortransparent.cpp
 * @brief CFML imagemakecolortransparent() built-in.
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

cfvariant *cf_imagemakecolortransparent(const cfvariant *image, const cfvariant *color)
{
    ImageData *src = image_from_variant(image);
    ImageData *dst = imageClone(src);
    uint32_t target = imgParseColor(color) & 0xFFFFFFu; // ignore alpha
    uint8_t tr = (uint8_t)((target >> 16) & 0xFF);
    uint8_t tg = (uint8_t)((target >> 8) & 0xFF);
    uint8_t tb = (uint8_t)(target & 0xFF);
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            uint8_t r, g, b, a;
            imgPixel(dst, x, y, &r, &g, &b, &a);
            if (r == tr && g == tg && b == tb) {
                // CF: matching pixel becomes fully transparent, RGB kept.
                surfaceSetRGBA(dst->surface, x, y, r, g, b, 0);
            }
        }
    }
    dst->colormodel = "argb";
    dst->colormodelType = "PackedColorModel";
    return imageResult(dst);
}

} // namespace cfml

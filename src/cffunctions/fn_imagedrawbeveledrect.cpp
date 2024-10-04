/**
 * @file fn_imagedrawbeveledrect.cpp
 * @brief CFML imagedrawbeveledrect() built-in.
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

cfvariant *cf_imagedrawbeveledrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                                   const cfvariant *width, const cfvariant *height, const cfvariant *raised,
                                   const cfvariant *filled)
{
    ImageData *img = image_from_variant(image);
    int X = toInt(x), Y = toInt(y), W = toInt(width), H = toInt(height);
    bool raisedFlag = toBool(raised), filledFlag = toBool(filled);

    uint32_t dc = img->drawingColor;
    auto tone = [](uint32_t c, double factor) -> uint32_t {
        uint32_t r = (uint32_t)std::min(255.0, std::floor(((c >> 16) & 0xFF) * factor));
        uint32_t g = (uint32_t)std::min(255.0, std::floor(((c >> 8) & 0xFF) * factor));
        uint32_t b = (uint32_t)std::min(255.0, std::floor((c & 0xFF) * factor));
        return (r << 16) | (g << 8) | b;
    };
    uint32_t bright = tone(dc, 1.0 / 0.7); // min(255, c / 0.7)
    uint32_t dark = tone(dc, 0.7);         // c * 0.7
    uint32_t tl = raisedFlag ? bright : dark;
    uint32_t br = raisedFlag ? dark : bright;

    // 1px frame over the outer bounds [X, X+W] x [Y, Y+H]. Java2D assigns the
    // corners to the vertical edges: the top-right corner keeps the right
    // edge's tone and the bottom-left corner keeps the left edge's tone.
    fillRectColor(img, X, Y, W, 1, tl);          // top (excl. top-right corner)
    fillRectColor(img, X + W, Y, 1, 1, br);      // top-right corner
    fillRectColor(img, X, Y + 1, 1, H - 1, tl);  // left (excl. corners)
    fillRectColor(img, X + W, Y + 1, 1, H - 1, br); // right (excl. corners)
    fillRectColor(img, X, Y + H, 1, 1, tl);      // bottom-left corner
    fillRectColor(img, X + 1, Y + H, W, 1, br);  // bottom (excl. bottom-left)
    if (filledFlag) {
        uint32_t inner = raisedFlag ? dc : dark;
        fillRectColor(img, X + 1, Y + 1, W - 1, H - 1, inner);
    }
    return nullResult();
}

} // namespace cfml

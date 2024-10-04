/**
 * @file fn_imagegrayscale.cpp
 * @brief CFML imagegrayscale() built-in.
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

static uint8_t luma(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b + 0.5);
}

cfvariant *cf_imagegrayscale(const cfvariant *image)
{
    ImageData *img = image_from_variant(image);
    imageTransform(img, [](uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) {
        uint8_t y = luma(r, g, b);
        r = g = b = y;
    });
    img->colormodel = "grayscale";
    img->colormodelType = "ComponentColorModel";
    return nullResult();
}

} // namespace cfml

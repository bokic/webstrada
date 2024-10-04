/**
 * @file fn_imagesetdrawingtransparency.cpp
 * @brief CFML imagesetdrawingtransparency() built-in.
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

cfvariant *cf_imagesetdrawingtransparency(const cfvariant *image, const cfvariant *percent)
{
    ImageData *img = image_from_variant(image);
    double p = toDouble(percent);
    if (p < 0 || p > 100) {
        imageThrow("Application", "The transparency value is a percentage. It should be between 0 and 100.",
                   "Verify your inputs. The transparency value is a percentage. It should be between 0 and 100.");
    }
    img->transparency = (int)p;
    img->xorMode = false; // last setter wins
    return nullResult();
}

} // namespace cfml

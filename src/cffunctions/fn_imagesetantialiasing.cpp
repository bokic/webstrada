/**
 * @file fn_imagesetantialiasing.cpp
 * @brief CFML imagesetantialiasing() built-in.
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

cfvariant *cf_imagesetantialiasing(const cfvariant *image, const cfvariant *antialias)
{
    ImageData *img = image_from_variant(image);
    if (antialias && antialias->m_type != cfvariant::Null) {
        std::string s = toLower(toStdString(antialias));
        if (s == "on" || s == "true") img->antialias = true;
        else if (s == "off" || s == "false") img->antialias = false;
        else imageThrow("Application", "The only acceptable values are on or off.",
                        "Verify your inputs. The only acceptable values are on or off.");
    }
    return nullResult();
}

} // namespace cfml

/**
 * @file fn_imagescaletofit.cpp
 * @brief CFML imagescaletofit() built-in.
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

cfvariant *cf_imagescaletofit(const cfvariant *image, const cfvariant *fitWidth, const cfvariant *fitHeight,
                              const cfvariant *interpolation, const cfvariant *blurFactor)
{
    ImageData *img = image_from_variant(image);
    std::string fw = fitWidth ? toStdString(fitWidth) : "";
    std::string fh = fitHeight ? toStdString(fitHeight) : "";
    std::string interp = (interpolation && interpolation->m_type != cfvariant::Null) ? toStdString(interpolation) : "highestquality";

    if (fw.empty()) {
        return cf_imageresize(image, nullptr, fitHeight, interpolation, blurFactor);
    }
    if (fh.empty()) {
        return cf_imageresize(image, fitWidth, nullptr, interpolation, blurFactor);
    }
    try {
        float scaleWidth = (float)atof(fw.c_str()) / img->width;
        float scaleHeight = (float)atof(fh.c_str()) / img->height;
        if (scaleWidth < scaleHeight) {
            return cf_imageresize(image, fitWidth, nullptr, interpolation, blurFactor);
        } else {
            return cf_imageresize(image, nullptr, fitHeight, interpolation, blurFactor);
        }
    } catch (...) {
        imageThrow("Application", "The fitWidth and fitHeight attribute should be positive and greater than zero for the scaleToFit operation.",
                   "Verify your inputs. The fitWidth and fitHeight attribute should be positive and greater than zero for the scaleToFit operation.");
    }
    return nullResult();
}

} // namespace cfml

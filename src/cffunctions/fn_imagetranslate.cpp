/**
 * @file fn_imagetranslate.cpp
 * @brief CFML imagetranslate() built-in.
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

cfvariant *cf_imagetranslate(const cfvariant *image, const cfvariant *xTrans, const cfvariant *yTrans,
                             const cfvariant *interpolation)
{
    ImageData *img = image_from_variant(image);
    int X = toInt(xTrans), Y = toInt(yTrans);
    if (interpolation && interpolation->m_type != cfvariant::Null) {
        std::string interp = toLower(toStdString(interpolation));
        if (interp != "nearest" && interp != "bilinear" && interp != "bicubic") {
            imageThrow("Application", "The interpolation argument must be one of: NEAREST|BILINEAR|BICUBIC",
                       "Verify your inputs. The interpolation argument must be one of: NEAREST|BILINEAR|BICUBIC");
        }
    }
    // CF: shift the image by (X,Y); the canvas keeps its size, clipped.
    int w = img->width, h = img->height;
    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(out);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, img->surface, X, Y);
    cairo_paint(cr);
    cairo_destroy(cr);
    imageReplaceSurface(img, out, w, h);
    return nullResult();
}

} // namespace cfml

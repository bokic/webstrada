/**
 * @file fn_imageresize.cpp
 * @brief CFML imageresize() built-in.
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

static void imageScale(ImageData *img, int outW, int outH, const std::string &interpolation)
{
    bool bilinear = (interpolation == "bilinear" || interpolation == "bicubic" ||
                     interpolation == "highestquality" || interpolation == "highquality" ||
                     interpolation == "mediumquality" || interpolation == "highperformance" ||
                     interpolation == "mediumperformance" || interpolation == "lowquality" ||
                     interpolation == "lowperformance");
    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, outW, outH);
    uint8_t r, g, b, a;
    for (int dy = 0; dy < outH; dy++) {
        for (int dx = 0; dx < outW; dx++) {
            double fx = dx * (double)img->width / outW;
            double fy = dy * (double)img->height / outH;
            samplePixel(img, fx, fy, bilinear, &r, &g, &b, &a);
            surfaceSetRGBA(out, dx, dy, r, g, b, a);
        }
    }
    imageReplaceSurface(img, out, outW, outH);
}

cfvariant *cf_imageresize(const cfvariant *image, const cfvariant *width, const cfvariant *height,
                          const cfvariant *interpolation, const cfvariant *blurFactor)
{
    ImageData *img = image_from_variant(image);
    std::string wStr = width ? toStdString(width) : "";
    std::string hStr = height ? toStdString(height) : "";
    wStr = wStr.empty() ? "" : wStr;
    hStr = hStr.empty() ? "" : hStr;

    if ((wStr.empty() && hStr.empty()) ||
        (width && width->m_type == cfvariant::Null && height && height->m_type == cfvariant::Null)) {
        imageThrow("Application", "Either height or width is undefined.",
                   "Verify your inputs. Either height or width is undefined.");
    }

    double blur = 1.0;
    if (blurFactor && blurFactor->m_type != cfvariant::Null) blur = toDouble(blurFactor);
    if (blur == 0.0) blur = 1.0;
    if (blur < 0.0 || blur > 10.0) {
        throw exception("Application", "The blur factor must be between 0 and 10.",
                        "Verify your inputs. The blur factor must be between 0 and 10.");
    }

    std::string interp = (interpolation && interpolation->m_type != cfvariant::Null) ? toLower(toStdString(interpolation)) : "highestquality";
    // Map the CF quality/performance names.
    if (interp == "highestperformance") interp = "bicubic";
    // Validate the interpolation name (CF's full accepted set).
    static const char *kValidInterp[] = {"nearest","bilinear","bicubic","bessel","blackman","hamming",
        "hanning","hermite","lanczos","mitchell","quadratic","highestquality","highquality",
        "mediumquality","highestperformance","highperformance","mediumperformance", nullptr};
    bool valid = false;
    for (int i = 0; kValidInterp[i]; i++) if (interp == kValidInterp[i]) { valid = true; break; }
    if (!valid) {
        imageThrow("Application", "The interpolation argument must be one of: NEAREST|BILINEAR|BICUBIC|BESSEL|BLACKMAN|HAMMING|HANNING|HERMITE|LANCZOS|MITCHELL|QUADRATIC|HIGHESTQUALITY|HIGHQUALITY|MEDIUMQUALITY|HIGHESTPERFORMANCE|HIGHPERFORMANCE|MEDIUMPERFORMANCE",
                   "Verify your inputs. The interpolation argument must be one of: NEAREST|BILINEAR|BICUBIC|BESSEL|BLACKMAN|HAMMING|HANNING|HERMITE|LANCZOS|MITCHELL|QUADRATIC|HIGHESTQUALITY|HIGHQUALITY|MEDIUMQUALITY|HIGHESTPERFORMANCE|HIGHPERFORMANCE|MEDIUMPERFORMANCE");
    }

    // Compute the scale factors (CF semantics: % and absolute).
    double scaleW = 1.0, scaleH = 1.0;
    if (!wStr.empty()) {
        if (wStr.back() == '%') scaleW = atof(wStr.substr(0, wStr.size()-1).c_str()) / 100.0;
        else scaleW = atof(wStr.c_str()) / img->width;
        if (scaleW <= 0.0) {
            imageThrow("Application", "The width attribute should be positive and greater than zero for the resize operation.",
                       "Verify your inputs. The width attribute should be positive and greater than zero for the resize operation.");
        }
        if (hStr.empty()) scaleH = scaleW;
    }
    if (!hStr.empty()) {
        if (hStr.back() == '%') scaleH = atof(hStr.substr(0, hStr.size()-1).c_str()) / 100.0;
        else scaleH = atof(hStr.c_str()) / img->height;
        if (scaleH <= 0.0) {
            imageThrow("Application", "The height attribute should be positive and greater than zero for the resize operation.",
                       "Verify your inputs. The height attribute should be positive and greater than zero for the resize operation.");
        }
        if (wStr.empty()) scaleW = scaleH;
    }

    if (scaleW * img->width * scaleH * img->height > 1.0E8f) {
        imageThrow("Application", "The cfimage tag does not support images larger than 100 megapixels. Resize the image to a lesser scale value.",
                   "Verify your inputs. The cfimage tag does not support images larger than 100 megapixels. Resize the image to a lesser scale value.");
    }
    if (scaleW == 1.0f && scaleH == 1.0f) return nullResult();

    int outW = (int)(scaleW * img->width + 0.5);
    int outH = (int)(scaleH * img->height + 0.5);
    if (outW < 1) outW = 1;
    if (outH < 1) outH = 1;

    // For nearest, use plain nearest sampling; others use bilinear.
    imageScale(img, outW, outH, interp);
    return nullResult();
}

} // namespace cfml

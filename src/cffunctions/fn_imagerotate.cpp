/**
 * @file fn_imagerotate.cpp
 * @brief CFML imagerotate() built-in.
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

cfvariant *cf_imagerotate(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                          const cfvariant *angle, const cfvariant *interpolation)
{
    ImageData *img = image_from_variant(image);
    // Robust to both the interpreter (correctly remapped) and the JIT (which
    // always passes the full 5-slot layout with nulls for missing args): when
    // `angle` is null the angle arrived in the `x` slot (JIT 2-arg call).
    const cfvariant *angPtr = angle;
    const cfvariant *xPtr = x;
    const cfvariant *yPtr = y;
    if (!angPtr || angPtr->m_type == cfvariant::Null) {
        angPtr = xPtr;
        xPtr = nullptr;
        yPtr = nullptr;
    }
    double ang = toDouble(angPtr);
    // CF's ImageRotate(image, angle) rotates about the image center; the
    // 5-arg form uses (x,y). Null x/y means the center.
    double px, py;
    if (xPtr && xPtr->m_type != cfvariant::Null && yPtr && yPtr->m_type != cfvariant::Null) {
        px = toDouble(xPtr);
        py = toDouble(yPtr);
    } else {
        px = img->width / 2.0;
        py = img->height / 2.0;
    }
    if (interpolation && interpolation->m_type != cfvariant::Null) {
        std::string interp = toLower(toStdString(interpolation));
        if (interp != "nearest" && interp != "bilinear" && interp != "bicubic") {
            imageThrow("Application", "The interpolation argument must be one of: NEAREST|BILINEAR|BICUBIC",
                       "Verify your inputs. The interpolation argument must be one of: NEAREST|BILINEAR|BICUBIC");
        }
    }

    double theta = ang * M_PI / 180.0;
    double cosA = cos(theta), sinA = sin(theta);
    // JAI "rotate" output size: the bounding box of the rotated rectangle,
    // newW = ceil(w*|cos| + h*|sin|), newH = ceil(w*|sin| + h*|cos|). The
    // rotation point stays fixed in the output; the image is rotated about it.
    double cx = px, cy = py;
    int outW = (int)ceil(img->width * fabs(cosA) + img->height * fabs(sinA) - 1e-9);
    int outH = (int)ceil(img->width * fabs(sinA) + img->height * fabs(cosA) - 1e-9);
    if (outW < 1) outW = 1;
    if (outH < 1) outH = 1;

    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, outW, outH);
    {
        cairo_t *cr = cairo_create(out);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, 0, 0, 0, 1); // opaque black background (CF)
        cairo_paint(cr);
        cairo_destroy(cr);
    }
    bool bilinear = (interpolation && interpolation->m_type != cfvariant::Null &&
                     toLower(toStdString(interpolation)) != "nearest");
    // Inverse map: source = R(-theta) * (output - center) + rotationPoint.
    double cosInv = cosA, sinInv = -sinA;
    double hw = outW / 2.0, hh = outH / 2.0;
    for (int dy = 0; dy < outH; dy++) {
        for (int dx = 0; dx < outW; dx++) {
            double tx = dx - hw, ty = dy - hh;
            double sx = cx + tx * cosInv - ty * sinInv;
            double sy = cy + tx * sinInv + ty * cosInv;
            if (sx < 0 || sy < 0 || sx >= img->width || sy >= img->height) continue;
            uint8_t r, g, b, a;
            samplePixel(img, sx, sy, bilinear, &r, &g, &b, &a);
            surfaceSetRGBA(out, dx, dy, r, g, b, a);
        }
    }
    imageReplaceSurface(img, out, outW, outH);
    return nullResult();
}

} // namespace cfml

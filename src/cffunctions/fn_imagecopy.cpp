/**
 * @file fn_imagecopy.cpp
 * @brief CFML imagecopy() built-in.
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

static ImageData *imageAllocBlank(int w, int h, const std::string &colormodel)
{
    auto *img = new ImageData;
    img->width = w;
    img->height = h;
    img->colormodel = colormodel;
    img->colormodelType = (colormodel == "grayscale") ? "ComponentColorModel" : "PackedColorModel";
    img->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    return img;
}

cfvariant *cf_imagecopy(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                        const cfvariant *width, const cfvariant *height,
                        const cfvariant *dx, const cfvariant *dy)
{
    ImageData *img = image_from_variant(image);
    int X = toInt(x), Y = toInt(y);
    int W = toInt(width), H = toInt(height);
    if (W < 0 || H < 0) {
        imageThrow("Application", "Height and width parameters should be non-negative.",
                   "Verify your inputs. Height and width parameters should be non-negative.");
    }
    // CF copies the rect from the source into a NEW image; when dx/dy are given
    // it also copies the rect within the source to (dx,dy).
    ImageData *copy = imageAllocBlank(W, H, img->colormodel);
    copy->colormodelType = img->colormodelType;
    copy->source = img->source;
    copy->sourceFormat = img->sourceFormat;
    {
        cairo_t *cr = cairo_create(copy->surface);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_surface(cr, img->surface, -X, -Y);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
    if (dx && dx->m_type != cfvariant::Null && dy && dy->m_type != cfvariant::Null) {
        // Copy the rect within the source to (dx, dy) (CF copyArea).
        int DX = toInt(dx), DY = toInt(dy);
        cairo_surface_t *tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
        {
            cairo_t *cr = cairo_create(tmp);
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(cr, img->surface, -X, -Y);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
        cairo_t *cr = cairo_create(img->surface);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_surface(cr, tmp, DX, DY);
        cairo_paint(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(tmp);
    }
    return imageResult(copy);
}

} // namespace cfml

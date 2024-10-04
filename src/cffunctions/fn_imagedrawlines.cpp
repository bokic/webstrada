/**
 * @file fn_imagedrawlines.cpp
 * @brief CFML imagedrawlines() built-in.
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

static std::vector<double> coordArray(const cfvariant *v)
{
    std::vector<double> out;
    if (!v || v->m_type != cfvariant::Array || !v->m_array) return out;
    for (const auto &item : *v->m_array) out.push_back(toDouble(&item));
    return out;
}

cfvariant *cf_imagedrawlines(const cfvariant *image, const cfvariant *xcords, const cfvariant *ycords,
                             const cfvariant *isPolygon, const cfvariant *filled)
{
    ImageData *img = image_from_variant(image);
    std::vector<double> xs = coordArray(xcords);
    std::vector<double> ys = coordArray(ycords);
    bool polygon = toBool(isPolygon), fill = toBool(filled);
    paintShape(img, [&](cairo_t *cr) {
        size_t n = std::min(xs.size(), ys.size());
        if (n == 0) return;
        cairo_move_to(cr, xs[0], ys[0]);
        for (size_t i = 1; i < n; i++) cairo_line_to(cr, xs[i], ys[i]);
        if (polygon) cairo_close_path(cr);
        if (fill) cairo_fill(cr); else cairo_stroke(cr);
    });
    return nullResult();
}

} // namespace cfml

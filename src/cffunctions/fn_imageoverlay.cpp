/**
 * @file fn_imageoverlay.cpp
 * @brief CFML imageoverlay() built-in.
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

static cairo_operator_t compositeRuleToOperator(const std::string &rule)
{
    if (rule == "src") return CAIRO_OPERATOR_SOURCE;
    if (rule == "dst") return CAIRO_OPERATOR_DEST;
    if (rule == "dst_over") return CAIRO_OPERATOR_DEST_OVER;
    if (rule == "src_in") return CAIRO_OPERATOR_IN;
    if (rule == "dst_in") return CAIRO_OPERATOR_DEST_IN;
    if (rule == "src_out") return CAIRO_OPERATOR_OUT;
    if (rule == "dst_out") return CAIRO_OPERATOR_DEST_OUT;
    return CAIRO_OPERATOR_OVER; // src_over default
}

cfvariant *cf_imageoverlay(const cfvariant *image1, const cfvariant *image2,
                           const cfvariant *rule, const cfvariant *alpha)
{
    ImageData *img1 = image_from_variant(image1);
    ImageData *img2 = image_from_variant(image2);
    std::string ruleStr = (rule && rule->m_type != cfvariant::Null) ? toLower(toStdString(rule)) : "src_over";
    if (ruleStr != "src" && ruleStr != "dst" && ruleStr != "dst_over" && ruleStr != "dst_in" &&
        ruleStr != "dst_out" && ruleStr != "src_in" && ruleStr != "src_over" && ruleStr != "src_out") {
        imageThrow("Application", "Invalid rule: " + ruleStr, "The rule argument is invalid.");
    }
    double a = (alpha && alpha->m_type != cfvariant::Null) ? atof(toStdString(alpha).c_str()) : 1.0;
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    cairo_t *cr = cairo_create(img1->surface);
    cairo_set_operator(cr, compositeRuleToOperator(ruleStr));
    cairo_set_source_surface(cr, img2->surface, 0, 0);
    // Alpha is applied via the source surface's alpha; a simple alpha scaling
    // is approximated by painting onto the surface (cairo handles the mask).
    cairo_paint_with_alpha(cr, a);
    cairo_destroy(cr);
    return nullResult();
}

} // namespace cfml

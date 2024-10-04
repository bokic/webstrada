/**
 * @file fn_imagesetdrawingstroke.cpp
 * @brief CFML imagesetdrawingstroke() built-in.
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

cfvariant *cf_imagesetdrawingstroke(const cfvariant *image, const cfvariant *attributes)
{
    ImageData *img = image_from_variant(image);
    if (attributes && attributes->m_type == cfvariant::Struct && attributes->m_struct) {
        if (const cfvariant *wv = structGet(attributes, "width")) {
            double w = toDouble(wv);
            if (w < 0) {
                imageThrow("Application", "The width must be greater than or equal to 0.0.",
                           "Verify your inputs. The width must be greater than or equal to 0.0.");
            }
            img->strokeWidth = w;
        }
        if (const cfvariant *cv = structGet(attributes, "endcaps")) {
            std::string c = toLower(toStdString(cv));
            if (c == "butt" || c == "round" || c == "square") img->strokeCaps = c;
        }
        if (const cfvariant *jv = structGet(attributes, "lineJoins")) {
            std::string j = toLower(toStdString(jv));
            if (j == "miter" || j == "round" || j == "bevel") img->strokeJoins = j;
        }
        if (const cfvariant *mv = structGet(attributes, "miterLimit")) img->strokeMiterLimit = toDouble(mv);
        if (const cfvariant *dv = structGet(attributes, "dashArray")) {
            if (dv->m_type == cfvariant::Array && dv->m_array) {
                img->strokeDash.clear();
                for (const auto &item : *dv->m_array) img->strokeDash.push_back(toDouble(&item));
            }
        }
        if (const cfvariant *pv = structGet(attributes, "dash_phases")) img->strokeDashPhase = toDouble(pv);
    }
    return nullResult();
}

} // namespace cfml

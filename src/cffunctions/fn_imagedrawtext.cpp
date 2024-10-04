/**
 * @file fn_imagedrawtext.cpp
 * @brief CFML imagedrawtext() built-in.
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

cfvariant *cf_imagedrawtext(const cfvariant *image, const cfvariant *str, const cfvariant *x,
                            const cfvariant *y, const cfvariant *attributes)
{
    ImageData *img = image_from_variant(image);
    std::string text = toStdString(str);
    double X = toDouble(x), Y = toDouble(y);

    std::string font = "Sans"; // Java2D default is the "Dialog" logical font;
                               // fontconfig substitutes an available family.
    double size = 10.0;
    bool bold = false, italic = false;
    bool underline = false, strikethrough = false;
    if (attributes && attributes->m_type == cfvariant::Struct && attributes->m_struct) {
        if (const cfvariant *fv = structGet(attributes, "font")) font = toStdString(fv);
        if (const cfvariant *sv = structGet(attributes, "size")) size = toDouble(sv);
        if (const cfvariant *st = structGet(attributes, "style")) {
            std::string style = toLower(toStdString(st));
            if (style == "bold") bold = true;
            else if (style == "italic") italic = true;
            else if (style == "bolditalic") { bold = true; italic = true; }
            else if (style != "plain" && style != "") {
                imageThrow("Application", "Invalid font style: " + toStdString(st),
                           "Verify your inputs. Invalid font style: " + toStdString(st));
            }
        }
        if (const cfvariant *uv = structGet(attributes, "underline")) underline = toBool(uv);
        if (const cfvariant *kv = structGet(attributes, "strikethrough")) strikethrough = toBool(kv);
    }

    // Text is rendered on the baseline starting at (X, Y), honoring the
    // drawing color/transparency/XOR mode and the drawing-axis transform.
    // Font metrics (and thus the exact glyph raster) differ between cairo and
    // Java2D, so ImageDrawText output cannot be pixel-verified against
    // ColdFusion (which also crashes when rendering any text on this server).
    paintShape(img, [&](cairo_t *cr) {
        cairo_select_font_face(cr, font.c_str(),
                               italic ? CAIRO_FONT_SLANT_OBLIQUE : CAIRO_FONT_SLANT_NORMAL,
                               bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, size);
        cairo_move_to(cr, X, Y);
        cairo_text_path(cr, text.c_str());
        cairo_fill(cr);
        if (underline || strikethrough) {
            cairo_font_extents_t fe;
            cairo_font_extents(cr, &fe);
            cairo_text_extents_t te;
            cairo_text_extents(cr, text.c_str(), &te);
            double x0 = X, x1 = X + te.x_advance;
            if (underline) {
                cairo_rectangle(cr, x0, Y + fe.descent / 2.0, x1 - x0, 1);
                cairo_fill(cr);
            }
            if (strikethrough) {
                cairo_rectangle(cr, x0, Y - fe.ascent * 0.45, x1 - x0, 1);
                cairo_fill(cr);
            }
        }
    });
    return nullResult();
}

} // namespace cfml

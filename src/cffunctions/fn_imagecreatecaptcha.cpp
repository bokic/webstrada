/**
 * @file fn_imagecreatecaptcha.cpp
 * @brief CFML imagecreatecaptcha() built-in.
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

static double vToDouble(const cfvariant *v)
{
    if (!v) return 0.0;
    switch (v->m_type) {
    case cfvariant::Number: return (double)v->m_int;
    case cfvariant::Long:   return (double)v->m_long;
    case cfvariant::Float:  return v->m_double;
    case cfvariant::Boolean: return v->m_bool ? 1.0 : 0.0;
    default: break;
    }
    std::string s = vToString(v);
    if (s.empty()) return 0.0;
    return strtod(s.c_str(), nullptr);
}

static int vToInt(const cfvariant *v) { return (int)vToDouble(v); }

[[noreturn]] static void captchaInvalidArg(const std::string &mesg)
{
    std::string detail = "Verify your inputs. " + mesg;
    throw exception(string("coldfusion.image.core.ImageExceptions$InvalidCaptchaArgumentException"),
                    string("An invalid argument has caused this error."),
                    string(detail.c_str()));
}

static ImageData *captchaMake(int width, int height, const std::string &text, int difficulty, CaptchaRng &rng)
{
    auto *img = new ImageData;
    img->width = width;
    img->height = height;
    img->colormodel = "argb";
    img->colormodelType = "PackedColorModel";
    img->source = "";
    img->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(img->surface);

    auto randomColor = [&](int minR, int maxV, int alphaRange, int alphaMin) {
        double r = (minR + rng.nextInt(maxV)) / 255.0;
        double g = rng.nextInt(maxV) / 255.0;
        double b = rng.nextInt(maxV) / 255.0;
        double a = (alphaMin + rng.nextInt(alphaRange)) / 255.0;
        cairo_set_source_rgba(cr, r, g, b, a);
    };

    // Background: a random light color or a two-point gradient.
    int r1 = 80 + rng.nextInt(176), g1 = rng.nextInt(176), b1 = rng.nextInt(176), a1 = rng.nextInt(256);
    if (rng.nextBoolean()) {
        int r2 = 80 + rng.nextInt(176), g2 = rng.nextInt(176), b2 = rng.nextInt(176), a2 = rng.nextInt(256);
        cairo_pattern_t *grad = cairo_pattern_create_linear(
            (double)rng.nextInt(width + 1), (double)rng.nextInt(height + 1),
            (double)rng.nextInt(width + 1), (double)rng.nextInt(height + 1));
        cairo_pattern_add_color_stop_rgba(grad, 0, r1 / 255.0, g1 / 255.0, b1 / 255.0, a1 / 255.0);
        // endColor.brighter(): lighten by 25%
        cairo_pattern_add_color_stop_rgba(grad, 1,
            std::min(1.0, (r2 / 255.0) * 1.25), std::min(1.0, (g2 / 255.0) * 1.25),
            std::min(1.0, (b2 / 255.0) * 1.25), a2 / 255.0);
        cairo_set_source(cr, grad);
        cairo_paint(cr);
        cairo_pattern_destroy(grad);
    } else {
        cairo_set_source_rgba(cr, r1 / 255.0, g1 / 255.0, b1 / 255.0, a1 / 255.0);
        cairo_paint(cr);
    }

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    if (difficulty > 0) {
        int numLines = rng.nextInt((51 * difficulty) / 2);
        for (int i = 0; i < numLines; i++) {
            CaptchaPoint p1{rng.nextInt(width + 1), rng.nextInt(height + 1)};
            CaptchaPoint p2{rng.nextInt(width + 1), rng.nextInt(height + 1)};
            CaptchaPoint p3{rng.nextInt(width + 1), rng.nextInt(height + 1)};
            cairo_set_line_width(cr, rng.nextInt(4) + 1);
            randomColor(80, 176, 256, 0);
            cairo_move_to(cr, p1.x, p1.y);
            cairo_curve_to(cr, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
            cairo_stroke(cr);
        }
    }

    if (difficulty > 1) {
        int numShapes = rng.nextInt(7);
        for (int i = 0; i < numShapes; i++) {
            int edges = 3 + rng.nextInt(7);
            randomColor(80, 176, 256, 0);
            cairo_set_line_width(cr, rng.nextInt(4) + 1);
            cairo_new_path(cr);
            for (int e = 0; e < edges; e++) {
                int px = rng.nextInt(width + 1), py = rng.nextInt(height + 1);
                if (e == 0) cairo_move_to(cr, px, py);
                else cairo_line_to(cr, px, py);
            }
            cairo_close_path(cr);
            if (rng.nextBoolean()) cairo_stroke(cr); else cairo_fill(cr);
        }
    }

    // Text: draw each glyph at a random position/rotation/color, like Java2D.
    cairo_set_font_face(cr, nullptr);
    double prevLetterEnd = 0.0;
    for (size_t i = 0; i < text.size(); i++) {
        char c[2] = {(char)text[i], 0};
        double angle = rng.nextDouble() - rng.nextDouble();
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 24.0);
        cairo_text_extents_t te;
        cairo_text_extents(cr, c, &te);
        double glyphW = te.width, glyphH = te.height;
        int remainingW = (int)std::floor((((double)width - glyphW) - prevLetterEnd) / (double)(text.size() - i));
        int x = remainingW <= 0 ? remainingW : rng.nextInt(remainingW);
        double thisX = (prevLetterEnd + x) - te.x_bearing;
        int remainingH = (int)std::floor((double)height - glyphH);
        int y = remainingH <= 0 ? remainingH : rng.nextInt(remainingH);
        double thisY = (glyphH + y) - te.y_bearing;
        randomColor(0, 50, 76, 180);
        cairo_save(cr);
        cairo_translate(cr, thisX, thisY);
        if (rng.nextBoolean()) cairo_rotate(cr, angle);
        cairo_move_to(cr, 0, 0);
        cairo_show_text(cr, c);
        cairo_restore(cr);
        prevLetterEnd = thisX + glyphW;
    }
    cairo_destroy(cr);
    return img;
}

cfvariant *cf_imagecreatecaptcha(const cfvariant *height, const cfvariant *width, const cfvariant *text,
                                 const cfvariant *difficulty, const cfvariant *font, const cfvariant *fontsize)
{
    int h = vToInt(height);
    int w = vToInt(width);
    std::string txt = vToString(text);
    std::string diff = (difficulty && difficulty->m_type != cfvariant::Null) ? vToLower(vToString(difficulty)) : "low";
    std::string fonts = (font && font->m_type != cfvariant::Null) ? vToString(font) : "";
    int fs = (fontsize && fontsize->m_type != cfvariant::Null) ? vToInt(fontsize) : 24;

    // ImageHelper.createCaptcha: zero dimensions get computed defaults.
    if (w == 0) w = (int)std::round((1.5 * (double)txt.length() * (double)fs * 0.72) + 1.0);
    if (h == 0) h = (int)std::round(((double)(2 * fs) * 0.72) + 1.0);

    if (txt.empty() || (size_t)txt.find_first_not_of(" \t\r\n") == std::string::npos) {
        captchaInvalidArg("The CAPTCHA text cannot be blank or undefined.");
    }
    if (w <= 0 || h <= 0 || fs <= 0) {
        captchaInvalidArg("The width, height and fontSize should be greater than zero to make a CAPTCHA.");
    }
    if ((double)w < 1.5 * (double)txt.length() * (double)fs * 0.72) {
        captchaInvalidArg("The specified width for the CAPTCHA image is not big enough to fit the text. Minimum width: " +
                          std::to_string((long long)std::round(1.5 * (double)txt.length() * (double)fs * 0.72)));
    }
    if ((double)h < ((double)(2 * fs)) * 0.72) {
        captchaInvalidArg("The specified height for captcha image is not big enough to fit the text. Minimum height: " +
                          std::to_string((long long)std::round(((double)(2 * fs)) * 0.72)));
    }

    int difficultyLevel;
    if (diff == "low") difficultyLevel = 0;
    else if (diff == "medium") difficultyLevel = 1;
    else if (diff == "high") difficultyLevel = 2;
    else {
        throw exception(string("coldfusion.image.core.ImageExceptions$UnsupportedCaptchaDifficultyException"),
                        string("The value of the difficulty attribute can be low, medium, or high."), string());
    }

    (void)fonts; // fonts are not available on the headless backend; ignored
    uint64_t seed = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    CaptchaRng rng(seed);
    ImageData *img = captchaMake(w, h, txt, difficultyLevel, rng);
    auto *ret = new cfvariant(cfvariant::Image);
    ret->m_image = img;
    return ret;
}

} // namespace cfml

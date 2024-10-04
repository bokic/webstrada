/**
 * @file fn_imagenew.cpp
 * @brief CFML imagenew() built-in.
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

static uint32_t colorName(const std::string &lower)
{
    static const std::map<std::string, uint32_t> colors = {
        {"black", 0xFF000000}, {"white", 0xFFFFFFFF}, {"red", 0xFFFF0000},
        {"lime", 0xFF00FF00}, {"green", 0xFF008000}, {"blue", 0xFF0000FF},
        {"yellow", 0xFFFFFF00}, {"cyan", 0xFF00FFFF}, {"aqua", 0xFF00FFFF},
        {"magenta", 0xFFFF00FF}, {"fuchsia", 0xFFFF00FF}, {"silver", 0xFFC0C0C0},
        {"gray", 0xFF808080}, {"grey", 0xFF808080}, {"maroon", 0xFF800000},
        {"olive", 0xFF808000}, {"navy", 0xFF000080}, {"teal", 0xFF008080},
        {"purple", 0xFF800080}, {"orange", 0xFFFFA500}, {"pink", 0xFFFFC0CB},
        {"brown", 0xFFA52A2A}, {"gold", 0xFFFFD700}, {"khaki", 0xFFF0E68C},
        {"lavender", 0xFFE6E6FA}, {"turquoise", 0xFF40E0D0}, {"violet", 0xFFEE82EE},
        {"indigo", 0xFF4B0082}, {"beige", 0xFFF5F5DC}, {"coral", 0xFFFF7F50},
        {"crimson", 0xFFDC143C}, {"salmon", 0xFFFA8072}, {"tan", 0xFFD2B48C},
        {"orchid", 0xFFDA70D6}, {"plum", 0xFFDDA0DD}, {"aquamarine", 0xFF7FFFD4},
        {"chartreuse", 0xFF7FFF00}, {"darkblue", 0xFF00008B}, {"darkcyan", 0xFF008B8B},
        {"darkgray", 0xFFA9A9A9}, {"darkgrey", 0xFFA9A9A9}, {"darkgreen", 0xFF006400},
        {"darkkhaki", 0xFFBDB76B}, {"darkmagenta", 0xFF8B008B}, {"darkolivegreen", 0xFF556B2F},
        {"darkorange", 0xFFFF8C00}, {"darkorchid", 0xFF9932CC}, {"darkred", 0xFF8B0000},
        {"darksalmon", 0xFFE9967A}, {"darkseagreen", 0xFF8FBC8F}, {"darkslateblue", 0xFF483D8B},
        {"darkslategray", 0xFF2F4F4F}, {"darkslategrey", 0xFF2F4F4F}, {"darkturquoise", 0xFF00CED1},
        {"darkviolet", 0xFF9400D3}, {"deeppink", 0xFFFF1493}, {"deepskyblue", 0xFF00BFFF},
        {"dimgray", 0xFF696969}, {"dimgrey", 0xFF696969}, {"firebrick", 0xFFB22222},
        {"forestgreen", 0xFF228B22}, {"hotpink", 0xFFFF69B4}, {"ivory", 0xFFFFFFF0},
        {"lightblue", 0xFFADD8E6}, {"lightcoral", 0xFFF08080}, {"lightcyan", 0xFFE0FFFF},
        {"lightgoldenrodyellow", 0xFFFAFAD2}, {"lightgray", 0xFFD3D3D3}, {"lightgrey", 0xFFD3D3D3},
        {"lightgreen", 0xFF90EE90}, {"lightpink", 0xFFFFB6C1}, {"lightsalmon", 0xFFFFA07A},
        {"lightseagreen", 0xFF20B2AA}, {"lightskyblue", 0xFF87CEFA}, {"lightslategray", 0xFF778899},
        {"lightslategrey", 0xFF778899}, {"lightsteelblue", 0xFFB0C4DE}, {"lightyellow", 0xFFFFFFE0},
        {"limegreen", 0xFF32CD32}, {"linen", 0xFFFAF0E6}, {"mediumblue", 0xFF0000CD},
        {"mediumorchid", 0xFFBA55D3}, {"mediumpurple", 0xFF9370DB}, {"mediumseagreen", 0xFF3CB371},
        {"mediumslateblue", 0xFF7B68EE}, {"mediumspringgreen", 0xFF00FA9A}, {"mediumturquoise", 0xFF48D1CC},
        {"mediumvioletred", 0xFFC71585}, {"midnightblue", 0xFF191970}, {"mintcream", 0xFFF5FFFA},
        {"mistyrose", 0xFFFFE4E1}, {"moccasin", 0xFFFFE4B5}, {"navajowhite", 0xFFFFDEAD},
        {"oldlace", 0xFFFDF5E6}, {"olivedrab", 0xFF6B8E23}, {"orangered", 0xFFFF4500},
        {"palegoldenrod", 0xFFEEE8AA}, {"palegreen", 0xFF98FB98}, {"paleturquoise", 0xFFAFEEEE},
        {"palevioletred", 0xFFDB7093}, {"papayawhip", 0xFFFFEFD5}, {"peachpuff", 0xFFFFDAB9},
        {"peru", 0xFFCD853F}, {"powderblue", 0xFFB0E0E6}, {"rebeccapurple", 0xFF663399},
        {"rosybrown", 0xFFBC8F8F}, {"royalblue", 0xFF4169E1}, {"saddlebrown", 0xFF8B4513},
        {"seagreen", 0xFF2E8B57}, {"seashell", 0xFFFFF5EE}, {"sienna", 0xFFA0522D},
        {"skyblue", 0xFF87CEEB}, {"slateblue", 0xFF6A5ACD}, {"slategray", 0xFF708090},
        {"slategrey", 0xFF708090}, {"snow", 0xFFFFFAFA}, {"springgreen", 0xFF00FF7F},
        {"steelblue", 0xFF4682B4}, {"thistle", 0xFFD8BFD8}, {"tomato", 0xFFFF6347},
        {"wheat", 0xFFF5DEB3}, {"whitesmoke", 0xFFF5F5F5}, {"yellowgreen", 0xFF9ACD32},
    };
    auto it = colors.find(lower);
    return it != colors.end() ? it->second : 0;
}

static uint32_t parseCanvasColor(const std::string &raw)
{
    std::string s = raw;
    size_t b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return 0xFF000000;
    s = s.substr(b);
    if (!s.empty() && s[0] == '#') s = s.substr(1);
    if (s.empty()) return 0xFF000000;
    bool hex = true;
    for (char c : s) {
        if (!isxdigit((unsigned char)c)) { hex = false; break; }
    }
    if (!hex) return colorName(toLower(s));
    if (s.size() == 3) {
        uint32_t r = (unsigned)std::strtoul(s.substr(0, 1).c_str(), nullptr, 16);
        uint32_t g = (unsigned)std::strtoul(s.substr(1, 1).c_str(), nullptr, 16);
        uint32_t bl = (unsigned)std::strtoul(s.substr(2, 1).c_str(), nullptr, 16);
        return 0xFF000000u | (r * 17) << 16 | (g * 17) << 8 | (bl * 17);
    }
    if (s.size() == 6) {
        uint32_t v = (uint32_t)std::strtoul(s.c_str(), nullptr, 16);
        return 0xFF000000u | v;
    }
    if (s.size() == 8) {
        return (uint32_t)std::strtoul(s.c_str(), nullptr, 16); // AARRGGBB
    }
    return 0xFF000000;
}

static void surfaceFill(cairo_surface_t *sf, uint32_t argb)
{
    cairo_t *cr = cairo_create(sf);
    double a = ((argb >> 24) & 0xFF) / 255.0;
    double r = ((argb >> 16) & 0xFF) / 255.0;
    double g = ((argb >> 8) & 0xFF) / 255.0;
    double b = (argb & 0xFF) / 255.0;
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_paint(cr);
    cairo_destroy(cr);
}

cfvariant *cf_imagenew(const cfvariant *source, const cfvariant *width, const cfvariant *height,
                       const cfvariant *imageType, const cfvariant *canvasColor)
{
    std::string type = imageType ? toStdString(imageType) : "";
    std::string cm = type.empty() ? "rgb" : toLower(type);
    if (cm != "rgb" && cm != "argb" && cm != "grayscale") {
        imageThrow("Application", "Image type must be RGB, ARGB, or GRAYSCALE.",
                   "Verify your inputs. Image type must be RGB, ARGB, or GRAYSCALE.");
    }

    std::string src = toStdString(source);
    if (source && source->m_type == cfvariant::Binary && source->m_binary) {
        ImageData *img = imageFromBytes(*source->m_binary, "", src);
        auto *ret = new cfvariant(cfvariant::Image);
        ret->m_image = img;
        return ret;
    }
    if (!src.empty()) {
        std::vector<std::byte> bytes;
        try {
            bytes = readFileBytes(src);
        } catch (...) {
            imageThrow("java.lang.IllegalArgumentException", "", "");
        }
        std::string fmt = sniffFormat(bytes);
        if (fmt.empty()) imageThrow("java.lang.IllegalArgumentException", "", "");
        ImageData *img = imageFromBytes(bytes, fmt, resolveSourcePath(src));
        auto *ret = new cfvariant(cfvariant::Image);
        ret->m_image = img;
        return ret;
    }

    int w = 32, h = 32;
    if (width && height) {
        w = toInt(width);
        h = toInt(height);
    } else if (width) {
        w = toInt(width);
    } else if (height) {
        h = toInt(height);
    }
    if (w <= 0 || h <= 0) {
        imageThrow("Application", "Height and width parameters should be non-negative.",
                   "Verify your inputs. Height and width parameters should be non-negative.");
    }

    uint32_t color = 0xFF000000u; // default canvas color is black
    if (canvasColor) {
        std::string cs = toStdString(canvasColor);
        if (!cs.empty()) color = parseCanvasColor(cs);
    }

    auto *img = new ImageData;
    img->width = w;
    img->height = h;
    img->colormodel = cm;
    img->colormodelType = (cm == "grayscale") ? "ComponentColorModel" : "PackedColorModel";
    img->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    surfaceFill(img->surface, color);

    auto *ret = new cfvariant(cfvariant::Image);
    ret->m_image = img;
    return ret;
}

} // namespace cfml

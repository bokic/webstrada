/**
 * @file fn_imagewritebase64.cpp
 * @brief CFML imagewritebase64() built-in.
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

namespace {

static const char kB64Tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

} // namespace

namespace cfml {

static std::string base64Encode(const std::vector<std::byte> &in)
{
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    auto push = [&](uint32_t v) { out += kB64Tab[v & 0x3F]; };
    while (i + 3 <= in.size()) {
        uint32_t v = ((uint32_t)(unsigned char)in[i] << 16) |
                     ((uint32_t)(unsigned char)in[i + 1] << 8) |
                     (uint32_t)(unsigned char)in[i + 2];
        push(v >> 18); push(v >> 12); push(v >> 6); push(v);
        i += 3;
    }
    size_t rem = in.size() - i;
    if (rem == 1) {
        uint32_t v = (uint32_t)(unsigned char)in[i] << 16;
        push(v >> 18); push(v >> 12); out += "==";
    } else if (rem == 2) {
        uint32_t v = ((uint32_t)(unsigned char)in[i] << 16) | ((uint32_t)(unsigned char)in[i + 1] << 8);
        push(v >> 18); push(v >> 12); push(v >> 6); out += "=";
    }
    return out;
}

static void writeFileText(const std::string &path, const std::string &text, bool overwrite)
{
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (char c : text) bytes.push_back((std::byte)c);
    writeFileBytes(path, bytes, overwrite);
}

cfvariant *cf_imagewritebase64(const cfvariant *image, const cfvariant *destination, const cfvariant *format,
                               const cfvariant *inHTMLFormat, const cfvariant *overwrite)
{
    ImageData *img = image_from_variant(image);
    std::string dest = toStdString(destination);
    std::string fmt = format ? toLower(toStdString(format)) : "";
    if (fmt.empty()) fmt = fileExt(dest);
    if (fmt.empty()) {
        imageThrow("Expression", "The source file should contain an extension,so that ColdFusion can determine the image format.", "");
    }
    std::vector<std::byte> bytes = encodeImage(img, fmt, 0.75);
    std::string b64 = base64Encode(bytes);
    bool html = inHTMLFormat ? toBool(inHTMLFormat) : false;
    std::string out = html ? ("data:image/" + fmt + ";base64," + b64) : b64;
    if (!dest.empty()) {
        bool ow = overwrite ? toBool(overwrite) : true;
        writeFileText(dest, out, ow);
    }
    auto *ret = new cfvariant(string(out.c_str()));
    return ret;
}

} // namespace cfml

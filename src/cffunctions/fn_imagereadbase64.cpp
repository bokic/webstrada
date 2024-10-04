/**
 * @file fn_imagereadbase64.cpp
 * @brief CFML imagereadbase64() built-in.
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

static int b64Val(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static std::vector<std::byte> base64Decode(const std::string &in)
{
    std::vector<std::byte> out;
    out.reserve(in.size() / 4 * 3);
    uint32_t acc = 0;
    int bits = 0;
    for (unsigned char c : in) {
        if (isspace(c)) continue;
        if (c == '=') break;
        int v = b64Val(c);
        if (v < 0) throw exception("Invalid base64 character");
        acc = (acc << 6) | (uint32_t)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back((std::byte)((acc >> bits) & 0xFF));
        }
    }
    return out;
}

cfvariant *cf_imagereadbase64(const cfvariant *data)
{
    if (!data) throw exception("ImageReadBase64 requires exactly 1 argument");
    std::string s = toStdString(data);
    size_t comma = s.find(',');
    if (s.rfind("data:", 0) == 0 && comma != std::string::npos) {
        s = s.substr(comma + 1);
    }
    std::vector<std::byte> bytes;
    try {
        bytes = base64Decode(s);
    } catch (...) {
        imageThrow("", "'' Can not decode string \"" + s + "\".", "");
    }
    std::string fmt = sniffFormat(bytes);
    if (fmt.empty()) {
        imageThrow("", "'' Can not decode string \"" + s + "\".", "");
    }
    ImageData *img = imageFromBytes(bytes, fmt, "");
    auto *ret = new cfvariant(cfvariant::Image);
    ret->m_image = img;
    return ret;
}

} // namespace cfml

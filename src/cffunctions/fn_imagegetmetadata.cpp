/**
 * @file fn_imagegetmetadata.cpp
 * @brief CFML imagegetmetadata() built-in.
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

static void buildPngMetadata(const std::vector<std::byte> &bytes, cfvariant &s)
{
    if (bytes.size() < 33) return;
    const uint8_t *p = (const uint8_t*)bytes.data();
    int w = ((int)p[16] << 24) | ((int)p[17] << 16) | ((int)p[18] << 8) | p[19];
    int h = ((int)p[20] << 24) | ((int)p[21] << 16) | ((int)p[22] << 8) | p[23];
    uint8_t depth = p[24];
    uint8_t ctype = p[25];
    uint8_t interlace = p[28];

    const char *colorType = "True Color";
    switch (ctype) {
    case 0: colorType = "Grayscale"; break;
    case 2: colorType = "True Color"; break;
    case 3: colorType = "Indexed Color"; break;
    case 4: colorType = "Gray With Alpha"; break;
    case 6: colorType = "True Color With Alpha"; break;
    default: break;
    }
    s.structSet("Bits Per Sample", cfvariant(string(std::to_string(depth).c_str())));
    s.structSet("Color Type", cfvariant(string(colorType)));
    s.structSet("Filter Method", cfvariant(string("Adaptive")));
    s.structSet("Image Width", cfvariant(string(std::to_string(w).c_str())));
    s.structSet("Image Height", cfvariant(string(std::to_string(h).c_str())));
    s.structSet("Interlace Method", cfvariant(string(interlace ? "Adam7 Interlace" : "No Interlace")));
    s.structSet("Compression Type", cfvariant(string("Deflate")));
}

static void buildJpegMetadata(const std::vector<std::byte> &bytes, cfvariant &s)
{
    const uint8_t *p = (const uint8_t*)bytes.data();
    size_t n = bytes.size();
    if (n < 4 || p[0] != 0xFF || p[1] != 0xD8) return;

    int precision = 8, width = 0, height = 0, ncomp = 0;
    bool baseline = false;
    struct JComp { int id, hv, qt; };
    std::vector<JComp> comps;
    bool hasJfif = false;
    int jfifMajor = 0, jfifMinor = 0, units = 0, xdensity = 1, ydensity = 1;

    size_t i = 2;
    while (i + 4 <= n) {
        if (p[i] != 0xFF) { i++; continue; }
        uint8_t code = p[i + 1];
        if (code == 0xD9 || code == 0xDA) break;
        if (code == 0x00 || (code >= 0xD0 && code <= 0xD7)) { i += 2; continue; }
        uint16_t len = (uint16_t)((p[i + 2] << 8) | p[i + 3]);
        if (len < 2 || i + 2 + len > n) break;
        if ((code >= 0xC0 && code <= 0xCF) && code != 0xC4 && code != 0xC8 && code != 0xCC) {
            baseline = (code == 0xC0);
            if (len >= 8) {
                precision = p[i + 4];
                height = (p[i + 5] << 8) | p[i + 6];
                width = (p[i + 7] << 8) | p[i + 8];
                ncomp = p[i + 9];
                comps.clear();
                for (int c = 0; c < ncomp && (size_t)(i + 10 + 3 * c + 2) < n; c++) {
                    JComp cc;
                    cc.id = p[i + 10 + 3 * c];
                    cc.hv = p[i + 11 + 3 * c];
                    cc.qt = p[i + 12 + 3 * c];
                    comps.push_back(cc);
                }
            }
        } else if (code == 0xE0 && len >= 16 && memcmp(p + i + 4, "JFIF\0", 5) == 0) {
            hasJfif = true;
            jfifMajor = p[i + 9];
            jfifMinor = p[i + 10];
            units = p[i + 11];
            xdensity = (p[i + 12] << 8) | p[i + 13];
            ydensity = (p[i + 14] << 8) | p[i + 15];
        }
        i += 2 + len;
    }

    if (!baseline && ncomp == 0) return;

    // CF's JPEG metadata struct iterates in the Java HashMap bucket order of
    // its capacity-32 map (ColdFusion's metadata map reaches 13+ entries and
    // resizes to 32). Emit insertion-ordered with that exact sequence so the
    // serialized output matches CF byte-for-byte.
    s.m_serializeInsertOrder = true;

    static const char *names[] = {"", "Y", "Cb", "Cr", "I", "Q"};
    auto compVariant = [&](int i) -> cfvariant {
        const char *name = (comps[i - 1].id >= 1 && comps[i - 1].id <= 5) ? names[comps[i - 1].id] : "?";
        int hf = (comps[i - 1].hv >> 4) & 0x0F;
        int vf = comps[i - 1].hv & 0x0F;
        std::string val = std::string(name) + " component: Quantization table " + std::to_string(comps[i - 1].qt) +
                          ", Sampling factors " + std::to_string(hf) + " horiz/" + std::to_string(vf) + " vert";
        return cfvariant(string(val.c_str()));
    };
    // Component 2 (bucket 0)
    if (ncomp >= 2) s.structSet("Component 2", compVariant(2));
    // Data Precision, then Component 1 (bucket 1)
    s.structSet("Data Precision", cfvariant(string((std::to_string(precision) + " bits").c_str())));
    if (ncomp >= 1) s.structSet("Component 1", compVariant(1));
    // X Resolution (bucket 6)
    if (hasJfif) s.structSet("X Resolution", cfvariant(string((std::to_string(xdensity) + " dot").c_str())));
    // Compression Type (bucket 15)
    s.structSet("Compression Type", cfvariant(string(baseline ? "Baseline" : "Extended Sequential Huffman")));
    // Resolution Units (bucket 22)
    if (hasJfif) s.structSet("Resolution Units", cfvariant(string(units == 1 ? "inch" : (units == 2 ? "cm" : "none"))));
    // Image Width, Y Resolution (bucket 23) — CF inserts Image Width first
    // (verified on CF 2025 via image_write_roundtrip.cfm).
    s.structSet("Image Width", cfvariant(string((std::to_string(width) + " pixels").c_str())));
    if (hasJfif) s.structSet("Y Resolution", cfvariant(string((std::to_string(ydensity) + " dot").c_str())));
    // Version (bucket 24)
    if (hasJfif) s.structSet("Version", cfvariant(string((std::to_string(jfifMajor) + "." + std::to_string(jfifMinor)).c_str())));
    // Image Height (bucket 26)
    s.structSet("Image Height", cfvariant(string((std::to_string(height) + " pixels").c_str())));
    // Component 4 (bucket 30)
    if (ncomp >= 4) s.structSet("Component 4", compVariant(4));
    // Number of Components, then Component 3 (bucket 31)
    s.structSet("Number of Components", cfvariant(string(std::to_string(ncomp).c_str())));
    if (ncomp >= 3) s.structSet("Component 3", compVariant(3));
    // Any further components share bucket 31 with Component 3; append after.
    for (int c = 5; c <= ncomp; c++) s.structSet(string(("Component " + std::to_string(c)).c_str()), compVariant(c));
}

cfvariant *cf_imagegetmetadata(const cfvariant *image)
{
    ImageData *img = image_from_variant(image);
    cfvariant s(cfvariant::Struct);
    if (img->sourceFormat == "png") {
        buildPngMetadata(img->sourceBytes, s);
    } else if (img->sourceFormat == "jpeg") {
        buildJpegMetadata(img->sourceBytes, s);
    } else if (img->sourceFormat == "gif") {
        s.structSet("Image Width", cfvariant(string(std::to_string(img->width).c_str())));
        s.structSet("Image Height", cfvariant(string(std::to_string(img->height).c_str())));
        s.structSet("Compression Type", cfvariant(string("LZW")));
    } else if (img->sourceFormat == "bmp") {
        s.structSet("Image Width", cfvariant(string(std::to_string(img->width).c_str())));
        s.structSet("Image Height", cfvariant(string(std::to_string(img->height).c_str())));
        s.structSet("Compression Type", cfvariant(string("None")));
        s.structSet("Bits Per Pixel", cfvariant(string("24")));
    } else if (img->sourceFormat == "pnm") {
        s.structSet("Image Width", cfvariant(string(std::to_string(img->width).c_str())));
        s.structSet("Image Height", cfvariant(string(std::to_string(img->height).c_str())));
    }
    auto *ret = new cfvariant(s);
    return ret;
}

} // namespace cfml

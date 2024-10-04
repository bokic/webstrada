/**
 * @file fn_imagegetiptctag.cpp
 * @brief CFML imagegetiptctag() built-in.
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

cfvariant *cf_imagegetiptctag(const cfvariant *name, const cfvariant *tagname)
{
    ImageData *img = image_from_variant(name);
    std::string want = vToString(tagname);
    cfvariant s(cfvariant::Struct);
    buildIptcStruct(img, s);
    std::string lowerWant = vToLower(want);
    if (s.m_struct) {
        for (auto &kv : *s.m_struct) {
            std::string key = kv.first.constData();
            if (vToLower(key) == lowerWant) {
                auto *ret = new cfvariant(kv.second);
                return ret;
            }
        }
    }
    auto *ret = new cfvariant(string(""));
    return ret;
}

} // namespace cfml

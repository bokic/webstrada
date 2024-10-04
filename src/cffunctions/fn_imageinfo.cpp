/**
 * @file fn_imageinfo.cpp
 * @brief CFML imageinfo() built-in.
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

static cfvariant boolVariant(bool b)
{
    cfvariant v(cfvariant::Boolean);
    v.m_bool = b;
    return v;
}

static cfvariant buildColormodelStruct(const std::string &colormodel, const std::string &colormodelType)
{
    cfvariant cm(cfvariant::Struct);
    cm.m_serializeInsertOrder = true;
    if (colormodel == "grayscale") {
        // CF-observed order (ComponentColorModel).
        cm.structSet("num_color_components", cfvariant(1));
        cm.structSet("colorspace", cfvariant(string("Any of the family of GRAY color spaces")));
        cm.structSet("pixel_size", cfvariant(8));
        cm.structSet("alpha_premultiplied", boolVariant(false));
        cm.structSet("transparency", cfvariant(string("OPAQUE")));
        cm.structSet("alpha_channel_support", boolVariant(false));
        cm.structSet("colormodel_type", cfvariant(string(colormodelType.c_str())));
        cm.structSet("bits_component_1", cfvariant(8));
        cm.structSet("num_components", cfvariant(1));
    } else if (colormodel == "argb") {
        // CF-observed order (PackedColorModel).
        cm.structSet("colorspace", cfvariant(string("Any of the family of RGB color spaces")));
        cm.structSet("pixel_size", cfvariant(32));
        cm.structSet("alpha_channel_support", boolVariant(true));
        cm.structSet("colormodel_type", cfvariant(string(colormodelType.c_str())));
        cm.structSet("num_components", cfvariant(4));
        cm.structSet("bits_component_4", cfvariant(8));
        cm.structSet("bits_component_3", cfvariant(8));
        cm.structSet("num_color_components", cfvariant(3));
        cm.structSet("alpha_premultiplied", boolVariant(false));
        cm.structSet("transparency", cfvariant(string("TRANSLUCENT")));
        cm.structSet("bits_component_2", cfvariant(8));
        cm.structSet("bits_component_1", cfvariant(8));
    } else {
        // rgb: shared key set for both ComponentColorModel (file reads) and
        // PackedColorModel (ImageNew "rgb").
        cm.structSet("bits_component_3", cfvariant(8));
        cm.structSet("num_color_components", cfvariant(3));
        cm.structSet("colorspace", cfvariant(string("Any of the family of RGB color spaces")));
        cm.structSet("pixel_size", cfvariant(24));
        cm.structSet("alpha_premultiplied", boolVariant(false));
        cm.structSet("transparency", cfvariant(string("OPAQUE")));
        cm.structSet("alpha_channel_support", boolVariant(false));
        cm.structSet("bits_component_2", cfvariant(8));
        cm.structSet("colormodel_type", cfvariant(string(colormodelType.c_str())));
        cm.structSet("bits_component_1", cfvariant(8));
        cm.structSet("num_components", cfvariant(3));
    }
    return cm;
}

cfvariant *cf_imageinfo(const cfvariant *image)
{
    ImageData *img = image_from_variant(image);
    cfvariant s(cfvariant::Struct);
    s.structSet("source", cfvariant(string(img->source.c_str())));
    s.structSet("width", cfvariant(img->width));
    s.structSet("height", cfvariant(img->height));
    s.structSet("colormodel", buildColormodelStruct(img->colormodel, img->colormodelType));
    auto *ret = new cfvariant(s);
    return ret;
}

} // namespace cfml

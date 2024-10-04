/**
 * @file tag_image.cpp
 * @brief <cfimage> tag runtime + ImageData payload helpers.
 *
 * Implements the <cfimage> tag runtime (cf_cfimage), reproducing
 * coldfusion.tagext.image.ImageTag: attribute validation, source loading
 * (Image var / Binary / base64 / path), the read / write / convert / resize /
 * border / rotate / info / writetobrowser / captcha actions, and the name /
 * structname variable assignment. Also defines the ImageData payload helpers
 * (image_data_retain / image_data_release / image_from_variant) declared in
 * include/webstrada/cfimage.h.
 */

#include <webstrada/cfimage.h>
#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>

#include <cairo.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace webstrada
{

ImageData *image_data_retain(ImageData *img)
{
    if (img) img->refcount++;
    return img;
}

void image_data_release(ImageData *img)
{
    if (img && --img->refcount <= 0) {
        if (img->surface) cairo_surface_destroy(img->surface);
        delete img;
    }
}

ImageData *image_from_variant(const cfvariant *v)
{
    if (!v || v->m_type != cfvariant::Image) {
        const char *cls = "java.lang.String";
        if (v) {
            switch (v->m_type) {
            case cfvariant::Number: cls = "java.lang.Number"; break;
            case cfvariant::Long: cls = "java.lang.Long"; break;
            case cfvariant::Float: cls = "java.lang.Double"; break;
            case cfvariant::Boolean: cls = "java.lang.Boolean"; break;
            case cfvariant::Struct: cls = "coldfusion.runtime.Struct"; break;
            case cfvariant::Array: cls = "coldfusion.runtime.Array"; break;
            case cfvariant::Query: cls = "coldfusion.sql.QueryTable"; break;
            case cfvariant::Binary: cls = "[B"; break;
            case cfvariant::Null: cls = "coldfusion.runtime.UndefinedVariableException"; break;
            default: break;
            }
        }
        throw exception(string("java.lang.ClassCastException"),
                        string(("class " + std::string(cls) + " cannot be cast to class coldfusion.image.Image").c_str()),
                        string());
    }
    return v->m_image;
}

} // namespace webstrada

namespace cfml {

namespace {

static std::string vToString(const cfvariant *v)
{
    if (!v) return "";
    webstrada::string tmp = const_cast<cfvariant*>(v)->toString();
    const char *d = tmp.constData();
    return d ? d : "";
}

static std::string vToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

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

static bool vToBool(const cfvariant *v)
{
    if (!v) return false;
    if (v->m_type == cfvariant::Boolean) return v->m_bool;
    std::string s = vToLower(vToString(v));
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    s = (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
    if (s.empty() || s == "false" || s == "no" || s == "off" || s == "0" || s == "null") return false;
    return true;
}

// Creates a Float cfvariant (cfvariant has no double constructor).
static cfvariant cfvariantFloat(double d)
{
    cfvariant v(cfvariant::Float);
    v.m_double = d;
    return v;
}

// Creates a Boolean cfvariant (cfvariant(bool) is the upcase/autocreate ctor).
static cfvariant cfvariantBool(bool b)
{
    cfvariant v(cfvariant::Boolean);
    v.m_bool = b;
    return v;
}

static const cfvariant *cfimageAttr(const cfvariant *attrs, const char *key)
{
    if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
    auto it = attrs->m_struct->find(string(key));
    return it != attrs->m_struct->end() ? &it->second : nullptr;
}

[[noreturn]] static void cfimageAppError(const char *message)
{
    throw exception(string("Application"), string(message), string());
}

// Writes an image to a temp file and returns the URI the tag emits. Mirrors
// CF's CFFileServlet/_cf_image + _cf_captcha temp locations and the
// `_cfimg<random>.<format>` / `_captcha_img<random>.png` file naming.
static std::string cfimageTempFile(const cfvariant *image, const std::string &folder,
                                   const std::string &prefix, const std::string &ext,
                                   const cfvariant *quality, bool overwrite)
{
    std::filesystem::path tmpDir = std::filesystem::temp_directory_path() / folder;
    std::filesystem::create_directories(tmpDir);
    uint64_t seed = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    seed ^= seed >> 33; seed *= 0xff51afd7ed558ccdULL; seed ^= seed >> 33;
    long long r1 = (long long)(seed & 0x7FFFFFFFFFFFFFFFLL);
    seed ^= seed >> 33; seed *= 0xc4ceb9fe1a85ec53ULL; seed ^= seed >> 33;
    long long r2 = (long long)(seed & 0x7FFFFFFFFFFFFFFFLL);
    std::string fileName = prefix + std::to_string(r1) + "." + ext;
    std::filesystem::path path = tmpDir / fileName;
    cfvariant dst(string(path.string().c_str()));
    cfvariant qv = quality ? *quality : cfvariantFloat(0.75);
    cfvariant ov(cfvariantBool(overwrite));
    cf_imagewrite(const_cast<cfvariant*>(image), &dst, &qv, &ov);
    return "/" + folder + "/" + fileName;
}

// Appends the writetobrowser / inline-captcha `<img ... />` tag. The writetobrowser
// form emits height/width (when attributes given) before `alt`; the captcha form
// emits `alt` first.
static void cfimageEmitImg(void *out, const std::string &uri,
                           const std::string &height, const std::string &width,
                           bool altBeforeDims)
{
    if (!out) return;
    if (cfml::response().binary) return; // cfcontent file/variable: other output ignored
    webstrada::string *outStr = static_cast<webstrada::string*>(out);
    std::string tag = "<img src=\"" + uri + "\"";
    if (altBeforeDims) tag += " alt=\"\"";
    if (!height.empty()) tag += " height=\"" + height + "\"";
    if (!width.empty()) tag += " width=\"" + width + "\"";
    if (!altBeforeDims) tag += " alt=\"\"";
    tag += " />\n";
    outStr->append(tag.c_str());
}

} // namespace

cfvariant *cf_cfimage(const cfvariant *attrs, void *variables, void *out)
{
    auto attrStr = [&](const char *key, const char *def) -> std::string {
        const cfvariant *v = cfimageAttr(attrs, key);
        if (!v || v->m_type == cfvariant::Null) return def;
        return vToString(v);
    };
    auto attrBool = [&](const char *key, bool def) -> bool {
        const cfvariant *v = cfimageAttr(attrs, key);
        if (!v || v->m_type == cfvariant::Null) return def;
        return vToBool(v);
    };
    auto attrInt = [&](const char *key, int def) -> int {
        const cfvariant *v = cfimageAttr(attrs, key);
        if (!v || v->m_type == cfvariant::Null) return def;
        return vToInt(v);
    };
    auto attrDouble = [&](const char *key, double def) -> double {
        const cfvariant *v = cfimageAttr(attrs, key);
        if (!v || v->m_type == cfvariant::Null) return def;
        return vToDouble(v);
    };

    std::string action = attrStr("action", "read");
    std::string actionL = vToLower(action);
    const cfvariant *sourceV = cfimageAttr(attrs, "source");
    const cfvariant *destV = cfimageAttr(attrs, "destination");
    const cfvariant *nameV = cfimageAttr(attrs, "name");
    bool isBase64 = attrBool("isbase64", false);
    std::string destination = (destV && destV->m_type != cfvariant::Null) ? vToString(destV) : "";
    std::string name = (nameV && nameV->m_type != cfvariant::Null) ? vToString(nameV) : "";
    double quality = attrDouble("quality", 0.75);
    bool overwrite = attrBool("overwrite", false);
    int thickness = attrInt("thickness", 1);
    int fontSize = attrInt("fontsize", 24);

    // Attribute validation, mirroring CF's per-attribute setters (a setter
    // only runs for attributes that actually appear in the tag).
    if (destV != nullptr && destV->m_type != cfvariant::Null && actionL != "captcha" && destination.empty())
        cfimageAppError("The destination attribute in the cfimage tag is either blank or undefined.");
    if (nameV != nullptr && nameV->m_type != cfvariant::Null && name.empty())
        cfimageAppError("The name attribute in the cfimage tag is either blank or undefined.");
    if (thickness < 0)
        cfimageAppError("The thickness attribute of a border cannot be negative.");
    if (fontSize <= 0)
        throw exception(string("java.lang.IllegalArgumentException"), string(), string());

    if (actionL == "captcha") {
        std::string text = attrStr("text", "");
        std::string wStr = attrStr("width", "");
        std::string hStr = attrStr("height", "");
        if (wStr.empty())
            wStr = std::to_string((int)std::round((1.5 * (double)text.length() * (double)fontSize * 0.72) + 1.0));
        if (hStr.empty())
            hStr = std::to_string((int)std::round(((double)(2 * fontSize) * 0.72) + 1.0));
        if (wStr.find_first_not_of("0123456789") != std::string::npos || wStr.empty())
            cfimageAppError("width should be a number.");
        if (hStr.find_first_not_of("0123456789") != std::string::npos || hStr.empty())
            cfimageAppError("height should be a number.");
        int w = atoi(wStr.c_str());
        int h = atoi(hStr.c_str());
        std::string diff = attrStr("difficulty", "low");
        std::string fonts = attrStr("fonts", "");
        cfvariant hv(h), wv(w), tv(string(text.c_str())), dv(string(diff.c_str())),
                  fv(string(fonts.c_str())), fsv(fontSize);
        cfvariant *cap = cf_imagecreatecaptcha(&hv, &wv, &tv, &dv, &fv, &fsv);
        if (!destination.empty()) {
            cfvariant qv(cfvariantFloat(quality)), ov(cfvariantBool(overwrite));
            cfvariant dstv(string(destination.c_str()));
            cf_imagewrite(cap, &dstv, &qv, &ov);
        }
        if (!name.empty()) {
            cfvariant *key = cfvariant_create_string(name.c_str());
            cfvariant_index_assign(static_cast<cfvariant*>(variables), key, cap);
        }
        if (destination.empty() && name.empty()) {
            std::string uri = cfimageTempFile(cap, "CFFileServlet/_cf_captcha", "_captcha_img", "png", nullptr, true);
            cfimageEmitImg(out, uri, std::to_string(h), std::to_string(w), true);
        }
        return nullptr;
    }

    // Non-captcha actions require a source.
    if (!sourceV || sourceV->m_type == cfvariant::Null) {
        cfimageAppError("The cfimage tag accepts only those ColdFusion variables that contain "
                        "Base64 strings, BLOBs, Byte arrays  or other images as inputs.");
    }

    // Load the working image. Always work on a copy so resize/border/rotate do
    // not mutate the caller's image (CF: new Image(source)).
    cfvariant work(cfvariant::Null);
    if (sourceV->m_type == cfvariant::Image) {
        cfvariant *imgTmp = cf_imageclone(sourceV);
        cf_register_temp(imgTmp);
        work = *imgTmp;
    } else if (sourceV->m_type == cfvariant::Binary) {
        cfvariant *imgTmp = cf_imagenew(sourceV, nullptr, nullptr, nullptr, nullptr);
        cf_register_temp(imgTmp);
        work = *imgTmp;
    } else if (sourceV->m_type == cfvariant::String) {
        cfvariant *imgTmp = isBase64 ? cf_imagereadbase64(sourceV) : cf_imageread(sourceV);
        cf_register_temp(imgTmp);
        work = *imgTmp;
    } else {
        cfimageAppError("The cfimage tag accepts only those ColdFusion variables that contain "
                        "Base64 strings, BLOBs, Byte arrays  or other images as inputs.");
    }

    std::string structname = attrStr("structname", "");
    std::string format = attrStr("format", "");

    if (actionL == "writetobrowser") {
        std::string fmt = format.empty() ? "PNG" : format;
        std::string uri = cfimageTempFile(&work, "CFFileServlet/_cf_image", "_cfimg", fmt, nullptr, true);
        cfimageEmitImg(out, uri, attrStr("height", ""), attrStr("width", ""), false);
    } else if (actionL == "resize") {
        std::string w = attrStr("width", "");
        std::string h = attrStr("height", "");
        cfvariant wv(string(w.c_str())), hv(string(h.c_str()));
        std::string interp = attrStr("interpolation", "highestquality");
        cfvariant iv(string(interp.c_str()));
        cfvariant *r = cf_imageresize(&work, w.empty() ? nullptr : &wv, h.empty() ? nullptr : &hv, &iv, nullptr);
        cf_register_temp(r);
    } else if (actionL == "border") {
        std::string color = attrStr("color", "black");
        cfvariant tv(thickness), cv(string(color.c_str())), typeV(string("constant"));
        cfvariant *r = cf_imageaddborder(&work, &tv, &cv, &typeV);
        cf_register_temp(r);
    } else if (actionL == "rotate") {
        int cx = work.m_image ? work.m_image->width / 2 : 0;
        int cy = work.m_image ? work.m_image->height / 2 : 0;
        cfvariant xv(cx), yv(cy), av(cfvariantFloat(attrDouble("angle", 0.0))), iv(string("bicubic"));
        cfvariant *r = cf_imagerotate(&work, &xv, &yv, &av, &iv);
        cf_register_temp(r);
    } else if (actionL == "info") {
        cfvariant *info = cf_imageinfo(&work);
        cf_register_temp(info);
        if (!structname.empty()) {
            cfvariant *key = cfvariant_create_string(structname.c_str());
            cf_register_temp(key);
            cfvariant_index_assign(static_cast<cfvariant*>(variables), key, info);
        }
    } else if (actionL != "read" && actionL != "write" && actionL != "convert") {
        throw exception(string("Application"),
                        string(("The action " + action + " is not supported by the cfimage tag.").c_str()),
                        string("Ensure that you have specified the correct action. Valid values are read, write, "
                               "writetobrowser, convert, resize, info, rotate, and border."));
    }

    // Write the result for write/convert/resize/border/rotate (info,
    // writetobrowser and read never write).
    if (actionL != "info" && actionL != "writetobrowser" && actionL != "read") {
        if (!destination.empty()) {
            cfvariant qv(cfvariantFloat(quality)), ov(cfvariantBool(overwrite)), dstv(string(destination.c_str()));
            cfvariant *r = cf_imagewrite(&work, &dstv, &qv, &ov);
            cf_register_temp(r);
        }
    }

    // Set the name variable (all actions).
    if (!name.empty()) {
        cfvariant *imgVar = new cfvariant(work);
        cf_register_temp(imgVar);
        cfvariant *key = cfvariant_create_string(name.c_str());
        cf_register_temp(key);
        cfvariant_index_assign(static_cast<cfvariant*>(variables), key, imgVar);
    }
    return nullptr;
}

} // namespace cfml

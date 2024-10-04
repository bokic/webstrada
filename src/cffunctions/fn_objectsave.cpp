/**
 * @file fn_objectsave.cpp
 * @brief CFML objectsave() / objectload() built-ins.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_objectsave(const cfvariant *obj, const cfvariant *file) {
    if (!obj) throw webstrada::exception("ObjectSave requires at least 1 argument");
    // Serialize to JSON (the engine's object wire format) and wrap the bytes.
    // CF uses Java Object Serialization; this engine cannot reproduce that
    // byte format, so ObjectSave/ObjectLoad round-trip through JSON within
    // this engine (see BUGS.md). A distinctive magic header distinguishes the
    // format.
    cfvariant *json = cf_serializejson(const_cast<cfvariant*>(obj), nullptr, nullptr, nullptr);
    cf_register_temp(json);
    webstrada::string jstr = json->toString();
    const char *jdata = jstr.constData();
    std::string js = jdata ? jdata : "";

    const char kMagic[] = "WEBSTRADA-OBJECT\0";
    constexpr size_t magicLen = sizeof(kMagic) - 1; // includes the embedded NUL terminator
    std::vector<std::byte> bytes;
    for (size_t i = 0; i < magicLen; i++) bytes.push_back(std::byte((unsigned char)kMagic[i]));
    for (char c : js) bytes.push_back(std::byte((unsigned char)c));

    if (file) {
        webstrada::string path = const_cast<cfvariant*>(file)->toString();
        std::ofstream of(path.constData(), std::ios::binary);
        if (!of) {
            throw webstrada::exception("ObjectSave", "Unable to write the serialized object to " + path + ".", "");
        }
        of.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    auto *ret = new cfvariant(cfvariant::Binary);
    *ret->m_binary = std::move(bytes);
    return ret;
}

cfvariant *cf_objectload(const cfvariant *binaryOrFile) {
    if (!binaryOrFile) throw webstrada::exception("ObjectLoad requires at least 1 argument");

    std::vector<std::byte> bytes;
    if (binaryOrFile->m_type == cfvariant::Binary && binaryOrFile->m_binary) {
        bytes = *binaryOrFile->m_binary;
    } else {
        webstrada::string path = const_cast<cfvariant*>(binaryOrFile)->toString();
        std::ifstream in(path.constData(), std::ios::binary);
        if (!in) {
            throw webstrada::exception("ObjectLoad", "Unable to read the serialized object from " + path + ".", "");
        }
        std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        bytes.resize(raw.size());
        for (size_t i = 0; i < raw.size(); i++) {
            bytes[i] = std::byte((unsigned char)raw[i]);
        }
    }

    // Strip the magic header.
    const char kMagic[] = "WEBSTRADA-OBJECT\0";
    size_t magicLen = sizeof(kMagic) - 1; // includes the embedded NUL terminator
    if (bytes.size() < magicLen) {
        throw webstrada::exception("ObjectLoad", "The data is not a valid serialized object.", "");
    }
    for (size_t i = 0; i < magicLen; i++) {
        if (bytes[i] != std::byte((unsigned char)kMagic[i])) {
            throw webstrada::exception("ObjectLoad", "The data is not a valid serialized object.", "");
        }
    }
    std::string js;
    for (size_t i = magicLen; i < bytes.size(); i++) {
        js += (char)bytes[i];
    }

    cfvariant jsonVal(js.c_str());
    return cf_deserializejson(&jsonVal, nullptr, true);
}

} // namespace cfml

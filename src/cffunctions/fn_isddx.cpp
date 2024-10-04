/**
 * @file fn_isddx.cpp
 * @brief CFML isddx() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <libxml/parser.h>
#include <string>

namespace cfml {

cfvariant *cf_isddx(const cfvariant *value) {
    if (!value) throw webstrada::exception("IsDDX requires exactly 1 argument");
    bool valid = false;
    if (value->m_type == cfvariant::String) {
        std::string xmlStr = safe_to_std_string(value->m_str);
        xmlDocPtr doc = xmlReadMemory(xmlStr.c_str(), xmlStr.length(), "isddx.xml",
                                      nullptr, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
        if (doc) {
            xmlNodePtr root = xmlDocGetRootElement(doc);
            if (root && root->name) {
                std::string rootName(reinterpret_cast<const char*>(root->name));
                // The DDX root element is <DDX xmlns="http://ns.adobe.com/DDX/1.0/">.
                valid = (rootName == "DDX");
            }
            xmlFreeDoc(doc);
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = valid;
    return ret;
}

} // namespace cfml

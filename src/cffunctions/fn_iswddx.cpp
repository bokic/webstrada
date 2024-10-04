/**
 * @file fn_iswddx.cpp
 * @brief CFML iswddx() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <libxml/parser.h>
#include <string>

namespace cfml {

cfvariant *cf_iswddx(const cfvariant *value) {
    if (!value) throw webstrada::exception("IsWDDX requires exactly 1 argument");
    bool valid = false;
    if (value->m_type == cfvariant::String) {
        std::string xmlStr = safe_to_std_string(value->m_str);
        xmlDocPtr doc = xmlReadMemory(xmlStr.c_str(), xmlStr.length(), "iswddx.xml",
                                      nullptr, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
        if (doc) {
            xmlNodePtr root = xmlDocGetRootElement(doc);
            if (root && root->name) {
                std::string rootName(reinterpret_cast<const char*>(root->name));
                // A valid WDDX packet needs the case-sensitive <wddxPacket>
                // root with a version attribute, a <header> element and at
                // least one value under <data> (verified against CF 2025:
                // missing version, uppercase root, missing <header/> or an
                // empty <data/> all return NO).
                bool hasVersion = false;
                bool hasHeader = false;
                bool hasDataChild = false;
                for (xmlAttrPtr a = root->properties; a; a = a->next) {
                    if (a->name && xmlStrEqual(a->name, BAD_CAST "version")) { hasVersion = true; break; }
                }
                for (xmlNodePtr ch = root->children; ch; ch = ch->next) {
                    if (ch->type != XML_ELEMENT_NODE || !ch->name) continue;
                    if (xmlStrEqual(ch->name, BAD_CAST "header")) {
                        hasHeader = true;
                    } else if (xmlStrEqual(ch->name, BAD_CAST "data")) {
                        for (xmlNodePtr dch = ch->children; dch; dch = dch->next) {
                            if (dch->type == XML_ELEMENT_NODE) { hasDataChild = true; break; }
                        }
                    }
                }
                valid = rootName == "wddxPacket" && hasVersion && hasHeader && hasDataChild;
            }
            xmlFreeDoc(doc);
        }
    }
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = valid;
    return ret;
}

} // namespace cfml

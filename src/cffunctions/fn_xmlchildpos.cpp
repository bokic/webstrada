/**
 * @file fn_xmlchildpos.cpp
 * @brief CFML xmlchildpos() built-in.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/xmlschemas.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_xmlchildpos(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2) {
    if (!arg0 || !arg1 || !arg2) throw webstrada::exception("XmlChildPos requires exactly 3 arguments");
    if (arg0->m_type != cfvariant::Xml) throw webstrada::exception("XmlChildPos: First argument must be an XML element");
    std::string childName = safe_to_std_string(*arg1);
    bool caseSensitive = arg0->m_upcase;
    int n = static_cast<int>(const_cast<cfvariant*>(arg2)->toString().toInt());

    int matchCount = 0;
    int idx = -1;
    if (arg0->m_struct->contains("XMLCHILDREN")) {
        cfvariant children = arg0->m_struct->at("XMLCHILDREN");
        if (children.m_type == cfvariant::Array) {
            for (size_t i = 0; i < children.m_array->size(); i++) {
                cfvariant child = children.m_array->at(i);
                std::string name = child.m_struct->contains("XMLNAME") ? safe_to_std_string(child.m_struct->at("XMLNAME")) : "";
                bool matches = caseSensitive ? (name == childName) : (strcasecmp(name.c_str(), childName.c_str()) == 0);
                if (matches) {
                    matchCount++;
                    if (matchCount == n) {
                        idx = static_cast<int>(i + 1);
                        break;
                    }
                }
            }
        }
    }
    auto *ret = new cfvariant(idx);
    return ret;
}

} // namespace cfml

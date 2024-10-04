/**
 * @file fn_xmlgetnodetype.cpp
 * @brief CFML xmlgetnodetype() built-in.
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

cfvariant *cf_xmlgetnodetype(const cfvariant *arg) {
    if (!arg) throw webstrada::exception("XmlGetNodeType requires exactly 1 argument");
    std::string type = "UNKNOWN";
    if (arg->m_type == cfvariant::Xml && arg->m_struct->contains("XMLTYPE")) {
        type = safe_to_std_string(arg->m_struct->at("XMLTYPE"));
    }
    // CF 2021 reports DOM node-type names, not the internal shorthand used by
    // this engine's XMLTYPE member (verified in tests/cfm/xml1.cfm).
    if (type == "ELEMENT") type = "ELEMENT_NODE";
    else if (type == "ATTRIBUTE") type = "ATTRIBUTE_NODE";
    else if (type == "TEXT") type = "TEXT_NODE";
    else if (type == "CDATA_SECTION") type = "CDATA_SECTION_NODE";
    else if (type == "ENTITY_REFERENCE") type = "ENTITY_REFERENCE_NODE";
    else if (type == "ENTITY") type = "ENTITY_NODE";
    else if (type == "PROCESSING_INSTRUCTION") type = "PROCESSING_INSTRUCTION_NODE";
    else if (type == "COMMENT") type = "COMMENT_NODE";
    else if (type == "DOCUMENT") type = "DOCUMENT_NODE";
    else if (type == "DOCUMENT_TYPE") type = "DOCUMENT_TYPE_NODE";
    auto *ret = new cfvariant(type.c_str());
    return ret;
}

} // namespace cfml

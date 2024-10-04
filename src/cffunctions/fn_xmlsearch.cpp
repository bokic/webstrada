/**
 * @file fn_xmlsearch.cpp
 * @brief CFML xmlsearch() built-in.
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

cfvariant *cf_xmlsearch(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2) {
    if (!arg0 || !arg1) throw webstrada::exception("XmlSearch requires at least 2 arguments");
    std::string xpath = safe_to_std_string(*arg1);

    std::string xmlStr = serialize_xml_node(*arg0);
    if (xmlStr.empty()) {
        if (arg0->m_type == cfvariant::Xml && arg0->m_struct->contains("XMLTYPE") && arg0->m_struct->at("XMLTYPE").toString().equals("ELEMENT")) {
            xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + serialize_xml_node(*arg0);
        }
    }

    cfvariant resArr;
    resArr.set_type(cfvariant::Array);

    xmlDocPtr doc = xmlReadMemory(xmlStr.c_str(), xmlStr.length(), "noname.xml", nullptr, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    if (doc) {
        xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
        xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)xpath.c_str(), xpathCtx);
        if (xpathObj && xpathObj->nodesetval) {
            for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++) {
                xmlNodePtr matchNode = xpathObj->nodesetval->nodeTab[i];
                cfvariant val = create_xml_node(matchNode, arg0->m_upcase);
                resArr.insert(val);
            }
        }
        if (xpathObj) xmlXPathFreeObject(xpathObj);
        if (xpathCtx) xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
    }

    auto *ret = new cfvariant(resArr);
    return ret;
}

} // namespace cfml

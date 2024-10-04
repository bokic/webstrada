/**
 * @file fn_xmltransform.cpp
 * @brief CFML xmltransform() built-in.
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

cfvariant *cf_xmltransform(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2) {
    if (!arg0 || !arg1) throw webstrada::exception("XmlTransform requires at least 2 arguments");
    std::string xmlStr = arg0->m_type == cfvariant::Xml ? serialize_xml_node(*arg0) : safe_to_std_string(*arg0);
    std::string xslStr = safe_to_std_string(*arg1);
    if (std::filesystem::exists(xslStr)) {
        std::ifstream f(xslStr);
        std::stringstream ss;
        ss << f.rdbuf();
        xslStr = ss.str();
    }

    xmlDocPtr xmlDoc = xmlReadMemory(xmlStr.c_str(), xmlStr.length(), "noname.xml", nullptr, 0);
    xmlDocPtr xslDoc = xmlReadMemory(xslStr.c_str(), xslStr.length(), "noname.xsl", nullptr, 0);

    std::string resStr = "";
    if (xmlDoc && xslDoc) {
        xsltStylesheetPtr cur = xsltParseStylesheetDoc(xslDoc);
        if (cur) {
            xmlDocPtr resDoc = xsltApplyStylesheet(cur, xmlDoc, nullptr);
            if (resDoc) {
                xmlChar *xmlResult = nullptr;
                int xmlResultLen = 0;
                xsltSaveResultToString(&xmlResult, &xmlResultLen, resDoc, cur);
                if (xmlResult) {
                    resStr = (const char*)xmlResult;
                    xmlFree(xmlResult);
                }
                xmlFreeDoc(resDoc);
            }
            xsltFreeStylesheet(cur);
        }
    }
    if (xmlDoc) xmlFreeDoc(xmlDoc);

    auto *ret = new cfvariant(resStr.c_str());
    return ret;
}

} // namespace cfml

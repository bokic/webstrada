/**
 * @file fn_xmlvalidate.cpp
 * @brief CFML xmlvalidate() built-in.
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

cfvariant *cf_xmlvalidate(const cfvariant *arg0, const cfvariant *arg1) {
    if (!arg0) throw webstrada::exception("XmlValidate requires at least 1 argument");
    std::string xmlStr = arg0->m_type == cfvariant::Xml ? serialize_xml_node(*arg0) : safe_to_std_string(*arg0);

    std::string schemaStr = arg1 ? safe_to_std_string(*arg1) : "";
    if (!schemaStr.empty() && std::filesystem::exists(schemaStr)) {
        std::ifstream f(schemaStr);
        std::stringstream ss;
        ss << f.rdbuf();
        schemaStr = ss.str();
    }

    bool isValid = true;
    cfvariant errorsArr;
    errorsArr.set_type(cfvariant::Array);

    // With no schema the document is not validated; CF 2021 reports status NO.
    if (schemaStr.empty()) {
        isValid = false;
    } else {
        xmlDocPtr doc = xmlReadMemory(xmlStr.c_str(), xmlStr.length(), "noname.xml", nullptr, 0);
        if (!doc) {
            isValid = false;
            errorsArr.insert(cfvariant("Failed to parse XML document"));
        } else {
            xmlSchemaParserCtxtPtr parser_ctxt = xmlSchemaNewMemParserCtxt(schemaStr.c_str(), schemaStr.length());
            xmlSchemaPtr schema = xmlSchemaParse(parser_ctxt);
            if (schema) {
                xmlSchemaValidCtxtPtr valid_ctxt = xmlSchemaNewValidCtxt(schema);
                int res = xmlSchemaValidateDoc(valid_ctxt, doc);
                if (res != 0) {
                    isValid = false;
                    errorsArr.insert(cfvariant("Document failed schema validation"));
                }
                xmlSchemaFreeValidCtxt(valid_ctxt);
                xmlSchemaFree(schema);
            }
            xmlSchemaFreeParserCtxt(parser_ctxt);
            xmlFreeDoc(doc);
        }
    }

    cfvariant resStruct;
    resStruct.set_type(cfvariant::Struct);
    {
        cfvariant statusVal(cfvariant::Boolean);
        statusVal.m_bool = isValid;
        resStruct.set("STATUS") = statusVal;
    }
    resStruct.set("errors") = errorsArr;
    resStruct.set("fatalerrors") = errorsArr;
    resStruct.set("warnings") = cfvariant(cfvariant::Array);

    auto *ret = new cfvariant(resStruct);
    return ret;
}

} // namespace cfml

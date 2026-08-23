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
namespace cfml {
using namespace webstrada;

namespace {

static xmlNodePtr build_libxml_tree(xmlDocPtr doc, cfvariant &var, std::vector<cfvariant*> &nodeTable) {
    if (var.m_type != cfvariant::Xml || !var.m_struct) return nullptr;

    std::string type = var.m_struct->contains("XMLTYPE") ? safe_to_std_string(var.m_struct->at("XMLTYPE").m_str) : "";
    if (type == "TEXT") {
        std::string val = var.m_struct->contains("XMLVALUE") ? safe_to_std_string(var.m_struct->at("XMLVALUE").m_str) : "";
        xmlNodePtr node = xmlNewDocText(doc, (const xmlChar*)val.c_str());
        if (node) {
            size_t idx = nodeTable.size();
            nodeTable.push_back(&var);
            node->_private = (void*)(uintptr_t)(idx + 1);
        }
        return node;
    } else if (type == "COMMENT") {
        std::string val = var.m_struct->contains("XMLVALUE") ? safe_to_std_string(var.m_struct->at("XMLVALUE").m_str) : "";
        xmlNodePtr node = xmlNewDocComment(doc, (const xmlChar*)val.c_str());
        if (node) {
            size_t idx = nodeTable.size();
            nodeTable.push_back(&var);
            node->_private = (void*)(uintptr_t)(idx + 1);
        }
        return node;
    } else if (type == "CDATA_SECTION") {
        std::string val = var.m_struct->contains("XMLVALUE") ? safe_to_std_string(var.m_struct->at("XMLVALUE").m_str) : "";
        xmlNodePtr node = xmlNewCDataBlock(doc, (const xmlChar*)val.c_str(), val.length());
        if (node) {
            size_t idx = nodeTable.size();
            nodeTable.push_back(&var);
            node->_private = (void*)(uintptr_t)(idx + 1);
        }
        return node;
    } else if (type == "ELEMENT") {
        std::string name = var.m_struct->contains("XMLNAME") ? safe_to_std_string(var.m_struct->at("XMLNAME").m_str) : "";
        if (name.empty()) name = "node";
        xmlNodePtr node = xmlNewDocNode(doc, nullptr, (const xmlChar*)name.c_str(), nullptr);
        if (node) {
            size_t idx = nodeTable.size();
            nodeTable.push_back(&var);
            node->_private = (void*)(uintptr_t)(idx + 1);
        }

        // Add attributes
        if (var.m_struct->contains("XMLATTRIBUTES")) {
            cfvariant &attrs = var.m_struct->at("XMLATTRIBUTES");
            if (attrs.m_type == cfvariant::Struct && attrs.m_struct) {
                if (attrs.m_structData && !attrs.m_structData->insertOrder.empty()) {
                    for (auto const& key : attrs.m_structData->insertOrder) {
                        if (attrs.m_struct->contains(key)) {
                            std::string attrName = safe_to_std_string(key);
                            std::string attrVal = safe_to_std_string(attrs.m_struct->at(key));
                            xmlNewProp(node, (const xmlChar*)attrName.c_str(), (const xmlChar*)attrVal.c_str());
                        }
                    }
                } else {
                    for (auto const& [key, val] : *attrs.m_struct) {
                        std::string attrName = safe_to_std_string(key);
                        std::string attrVal = safe_to_std_string(val);
                        xmlNewProp(node, (const xmlChar*)attrName.c_str(), (const xmlChar*)attrVal.c_str());
                    }
                }
            }
        }

        // Build children: use XMLNODES (all node types: text, element, comment)
        // first, then append any extra elements from XMLCHILDREN that are not
        // already included in XMLNODES. This covers user-appended nodes via
        // arrayAppend(node.xmlChildren, ...) which updates XMLCHILDREN but not XMLNODES.
        size_t nodesCount = 0;
        if (var.m_struct->contains("XMLNODES") && var.m_struct->at("XMLNODES").m_type == cfvariant::Array && var.m_struct->at("XMLNODES").m_array) {
            for (auto &child : *var.m_struct->at("XMLNODES").m_array) {
                xmlNodePtr childNode = build_libxml_tree(doc, child, nodeTable);
                if (childNode) {
                    xmlAddChild(node, childNode);
                }
                nodesCount++;
            }
        }
        // Append extra elements from XMLCHILDREN not already covered by XMLNODES
        if (var.m_struct->contains("XMLCHILDREN") && var.m_struct->at("XMLCHILDREN").m_type == cfvariant::Array && var.m_struct->at("XMLCHILDREN").m_array) {
            size_t childrenCount = var.m_struct->at("XMLCHILDREN").m_array->size();
            // Count element nodes in XMLNODES
            size_t elemNodesCount = 0;
            if (var.m_struct->contains("XMLNODES") && var.m_struct->at("XMLNODES").m_type == cfvariant::Array && var.m_struct->at("XMLNODES").m_array) {
                for (auto &n : *var.m_struct->at("XMLNODES").m_array) {
                    if (n.m_type == cfvariant::Xml && n.m_struct) {
                        auto itType = n.m_struct->find("XMLTYPE");
                        if (itType != n.m_struct->end() && safe_to_std_string(itType->second.m_str) == "ELEMENT") {
                            elemNodesCount++;
                        }
                    }
                }
            }
            // Append children that exceed what XMLNODES covers
            if (childrenCount > elemNodesCount) {
                for (size_t ci = elemNodesCount; ci < childrenCount; ci++) {
                    auto &child = var.m_struct->at("XMLCHILDREN").m_array->at(ci);
                    xmlNodePtr childNode = build_libxml_tree(doc, child, nodeTable);
                    if (childNode) {
                        xmlAddChild(node, childNode);
                    }
                }
            }
        }

        if (nodesCount == 0 && (!var.m_struct->contains("XMLCHILDREN") || var.m_struct->at("XMLCHILDREN").m_array->empty())) {
            // Leaf element: add text content from XMLTEXT
            if (var.m_struct->contains("XMLTEXT")) {
                std::string txt = safe_to_std_string(var.m_struct->at("XMLTEXT").m_str);
                if (!txt.empty()) {
                    xmlNodeAddContent(node, (const xmlChar*)txt.c_str());
                }
            }
        }

        return node;
    } else if (type == "DOCUMENT") {
        if (var.m_struct->contains("XMLROOT")) {
            cfvariant &root = var.m_struct->at("XMLROOT");
            xmlNodePtr rootNode = build_libxml_tree(doc, root, nodeTable);
            if (rootNode) {
                xmlDocSetRootElement(doc, rootNode);
            }
        }
        return (xmlNodePtr)doc;
    }
    return nullptr;
}

} // namespace

cfvariant *cf_xmlsearch(const cfvariant *arg0, const cfvariant *arg1, const cfvariant *arg2) {
    if (!arg0 || !arg1) throw webstrada::exception("XmlSearch requires at least 2 arguments");
    std::string xpath = safe_to_std_string(*arg1);

    cfvariant resArr;
    resArr.set_type(cfvariant::Array);

    if (arg0->m_type == cfvariant::Xml) {
        std::vector<cfvariant*> nodeTable;
        xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
        xmlNodePtr rootNode = nullptr;

        std::string type = (arg0->m_struct && arg0->m_struct->contains("XMLTYPE"))
            ? safe_to_std_string(arg0->m_struct->at("XMLTYPE").m_str) : "";

        if (type == "DOCUMENT") {
            build_libxml_tree(doc, const_cast<cfvariant&>(*arg0), nodeTable);
        } else {
            rootNode = build_libxml_tree(doc, const_cast<cfvariant&>(*arg0), nodeTable);
            if (rootNode) {
                xmlDocSetRootElement(doc, rootNode);
            }
        }

        xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
        if (rootNode) {
            xpathCtx->node = rootNode;
        }

        xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)xpath.c_str(), xpathCtx);
        if (xpathObj && xpathObj->nodesetval) {
            for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++) {
                xmlNodePtr matchNode = xpathObj->nodesetval->nodeTab[i];
                uintptr_t idx = (uintptr_t)matchNode->_private;
                if (idx > 0 && idx <= nodeTable.size()) {
                    resArr.insert(*nodeTable[idx - 1]);
                } else {
                    cfvariant val = create_xml_node(matchNode, arg0->m_upcase);
                    resArr.insert(val);
                }
            }
        }
        if (xpathObj) xmlXPathFreeObject(xpathObj);
        if (xpathCtx) xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
    } else {
        std::string xmlStr = safe_to_std_string(*arg0);
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
    }

    auto *ret = new cfvariant(resArr);
    return ret;
}

} // namespace cfml

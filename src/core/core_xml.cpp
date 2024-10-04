#include "core_internal.h"
#include "../cftags/common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
#include <webstrada/worker.h>
#include <webstrada/cfimage.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/scope_store.h>
#include <webstrada/config.h>
#include <webstrada/locale.h>
#include <webstrada/cfimage.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>

#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>

using namespace webstrada;
using namespace cfml;
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <unistd.h>
#include <fcntl.h>

// ---- XML helpers ----

cfvariant cfml::create_xml_node(xmlNodePtr node, bool caseSensitive) {
    cfvariant var(!caseSensitive, false);
    var.set_type(cfvariant::Xml);

    // XMLNAME: CF reports the DOM node-name, which for special node types is
    // the fixed names "#text" / "#comment" / "#cdata-section", not libxml's
    // internal "text" / "comment" / "cdatasection" (verified on the RDS host).
    std::string name;
    std::string typeStr;
    switch (node->type) {
        case XML_ELEMENT_NODE: typeStr = "ELEMENT"; name = (node->name ? (const char*)node->name : ""); break;
        case XML_ATTRIBUTE_NODE: typeStr = "ATTRIBUTE"; name = (node->name ? (const char*)node->name : ""); break;
        case XML_TEXT_NODE: typeStr = "TEXT"; name = "#text"; break;
        case XML_CDATA_SECTION_NODE: typeStr = "CDATA_SECTION"; name = "#cdata-section"; break;
        case XML_ENTITY_REF_NODE: typeStr = "ENTITY_REFERENCE"; name = (node->name ? (const char*)node->name : ""); break;
        case XML_ENTITY_NODE: typeStr = "ENTITY"; name = (node->name ? (const char*)node->name : ""); break;
        case XML_PI_NODE: typeStr = "PROCESSING_INSTRUCTION"; name = (node->name ? (const char*)node->name : ""); break;
        case XML_COMMENT_NODE: typeStr = "COMMENT"; name = "#comment"; break;
        case XML_DOCUMENT_NODE: typeStr = "DOCUMENT"; name = "#document"; break;
        default: typeStr = "UNKNOWN"; name = (node->name ? (const char*)node->name : ""); break;
    }
    var.set("XMLNAME") = cfvariant(name.c_str());
    var.set("XMLTYPE") = cfvariant(typeStr.c_str());

    // XMLVALUE: CF reads node.getNodeValue(); for element/document nodes that
    // is null, so the value is empty. Text/CDATA/Comment nodes carry their
    // content as the value.
    std::string valStr;
    if (node->type == XML_TEXT_NODE || node->type == XML_CDATA_SECTION_NODE ||
        node->type == XML_COMMENT_NODE || node->type == XML_PI_NODE ||
        node->type == XML_ENTITY_REF_NODE || node->type == XML_ENTITY_NODE) {
        xmlChar *content = xmlNodeGetContent(node);
        if (content) {
            valStr = (const char*)content;
            xmlFree(content);
        }
    }
    var.set("XMLVALUE") = cfvariant(valStr.c_str());

    // XMLTEXT: CF's XmlFunctions.getText concatenates only the element's DIRECT
    // text/CDATA child node values (not descendant element text), and is only
    // exposed on element nodes (verified: <root>pre<a>1</a></root> -> "pre").
    if (node->type == XML_ELEMENT_NODE) {
        std::string textVal;
        for (xmlNodePtr child = node->children; child != nullptr; child = child->next) {
            if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE) {
                xmlChar *c = xmlNodeGetContent(child);
                if (c) {
                    textVal += (const char*)c;
                    xmlFree(c);
                }
            }
        }
        var.set("XMLTEXT") = cfvariant(textVal.c_str());
    }

    cfvariant attrs(caseSensitive, false);
    attrs.set_type(cfvariant::Struct);
    if (node->type == XML_ELEMENT_NODE) {
        for (xmlAttrPtr attr = node->properties; attr != nullptr; attr = attr->next) {
            std::string attrName = (const char*)attr->name;
            xmlChar *attrVal = xmlGetProp(node, attr->name);
            std::string attrValStr = (attrVal ? (const char*)attrVal : "");
            if (attrVal) xmlFree(attrVal);
            
            string keyName(attrName.c_str());
            if (!caseSensitive) keyName.toUpper();
            attrs.set(keyName) = cfvariant(attrValStr.c_str());
        }
    }
    var.set("XMLATTRIBUTES") = attrs;

    cfvariant children(caseSensitive, false);
    children.set_type(cfvariant::Array);
    
    cfvariant nodes(caseSensitive, false);
    nodes.set_type(cfvariant::Array);

    std::map<string, std::vector<cfvariant>> childrenByName;

    for (xmlNodePtr child = node->children; child != nullptr; child = child->next) {
        cfvariant childVar = create_xml_node(child, caseSensitive);
        nodes.insert(childVar);

        if (child->type == XML_ELEMENT_NODE) {
            children.insert(childVar);
            
            std::string childTagName = (const char*)child->name;
            string keyTagName(childTagName.c_str());
            if (!caseSensitive) {
                keyTagName.toUpper();
            }
            childrenByName[keyTagName].push_back(childVar);
        }
    }

    var.set("XMLCHILDREN") = children;
    var.set("XMLNODES") = nodes;

    // CF exposes same-name child elements under their tag name: a single
    // occurrence resolves to the element itself, multiple ones to an array
    // (XmlNodeArray). Verified on the RDS host: <root><Child>x</Child></root>
    // gives root.CHILD the single element, <root><Child>1</Child><Child>2</Child></root>
    // gives root.CHILD a 2-element array.
    for (auto const& [name, list] : childrenByName) {
        if (list.size() == 1) {
            var.set(name) = list[0];
        } else {
            cfvariant childGroup(caseSensitive, false);
            childGroup.set_type(cfvariant::Array);
            // CF wraps the multi-child group in an XmlNodeArray: a Java List
            // that IsArray() rejects but len()/ArrayLen()/indexing accept.
            childGroup.m_isXmlNodeList = true;
            for (auto const& item : list) {
                childGroup.insert(item);
            }
            var.set(name) = childGroup;
        }
    }

    return var;
}

cfvariant cfml::create_xml_document(xmlDocPtr doc, bool caseSensitive) {
    cfvariant var(!caseSensitive, false);
    var.set_type(cfvariant::Xml);
    var.set("XMLTYPE") = cfvariant("DOCUMENT");
    
    xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    if (rootNode) {
        cfvariant rootVar = create_xml_node(rootNode, caseSensitive);
        rootVar.set("XMLISROOT") = cfvariant(1);
        var.set("XMLROOT") = rootVar;
        std::string rootName = (const char*)rootNode->name;
        string keyRootName(rootName.c_str());
        if (!caseSensitive) {
            keyRootName.toUpper();
        }
        var.set(keyRootName) = rootVar;
    }
    
    return var;
}

std::string cfml::safe_to_std_string(const webstrada::string *s) {
    if (!s) return "";
    const char *d = s->constData();
    return d ? d : "";
}

std::string cfml::safe_to_std_string(const webstrada::string &s) {
    const char *d = s.constData();
    return d ? d : "";
}

std::string cfml::safe_to_std_string(const cfvariant &v) {
    // Bind the temporary: constData() on a temporary string dangles once the
    // temporary is destroyed (COW string), so computed strings (query-column
    // cells, numbers) would be read from freed memory.
    webstrada::string s = const_cast<cfvariant&>(v).toString();
    const char *d = s.constData();
    return d ? d : "";
}

std::string cfml::serialize_xml_node(const cfvariant &node) {
    if (node.m_type != cfvariant::Xml) return "";

    std::string type = node.m_struct->contains("XMLTYPE") ? safe_to_std_string(node.m_struct->at("XMLTYPE").m_str) : "";
    if (type == "TEXT") {
        return node.m_struct->contains("XMLVALUE") ? safe_to_std_string(node.m_struct->at("XMLVALUE").m_str) : "";
    } else if (type == "COMMENT") {
        std::string val = node.m_struct->contains("XMLVALUE") ? safe_to_std_string(node.m_struct->at("XMLVALUE").m_str) : "";
        return "<!--" + val + "-->";
    } else if (type == "ELEMENT") {
        std::string name = node.m_struct->contains("XMLNAME") ? safe_to_std_string(node.m_struct->at("XMLNAME").m_str) : "";
        std::string res = "<" + name;
        
        if (node.m_struct->contains("XMLATTRIBUTES")) {
            cfvariant attrs = node.m_struct->at("XMLATTRIBUTES");
            if (attrs.m_type == cfvariant::Struct) {
                for (auto const& [key, val] : *attrs.m_struct) {
                    res += " " + safe_to_std_string(key) + "=\"" + safe_to_std_string(val) + "\"";
                }
            }
        }
        
        if (node.m_struct->contains("XMLNODES")) {
            cfvariant children = node.m_struct->at("XMLNODES");
            if (children.m_type == cfvariant::Array && !children.m_array->empty()) {
                res += ">";
                for (auto const& child : *children.m_array) {
                    res += serialize_xml_node(child);
                }
                res += "</" + name + ">";
            } else {
                res += "/>";
            }
        } else {
            res += "/>";
        }
        return res;
    } else if (type == "DOCUMENT") {
        std::string res = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        if (node.m_struct->contains("XMLROOT")) {
            res += serialize_xml_node(node.m_struct->at("XMLROOT"));
        }
        return res;
    }
    return "";
}

string cfml::readfile(const char *pathname)
{
    char tmp[1024];
    int nread = 0;
    string ret;

    int fd = open(pathname, O_RDONLY);

    if (fd < 0) {
        return ret;
    }

    while ((nread = read(fd, tmp, sizeof(tmp))) > 0) {
        ret.append(tmp, nread);
    }

    if (nread == -1) {
        ret.clear();
    }

    close(fd);

    return ret;
}


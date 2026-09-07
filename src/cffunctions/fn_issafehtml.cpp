/**
 * @file fn_issafehtml.cpp
 * @brief CFML isSafeHTML() built-in function.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using webstrada::cfvariant;
using webstrada::string;

namespace {

static const char *DEFAULT_ANTISAMY_POLICY = R"XML(<?xml version="1.0" encoding="ISO-8859-1"?>
<anti-samy-rules xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
		xsi:noNamespaceSchemaLocation="antisamy.xsd">
	<directives>
		<directive name="omitXmlDeclaration" value="true"/>
		<directive name="omitDoctypeDeclaration" value="true"/>
		<directive name="maxInputSize" value="100000"/>
		<directive name="useXHTML" value="true"/>
		<directive name="formatOutput" value="false"/>
		<directive name="embedStyleSheets" value="false"/>
	</directives>
	<common-regexps>
		<regexp name="htmlTitle" value="[\p{L}\p{N}\s\-_',:\[\]!\./\\\(\)&amp;]*"/>
		<regexp name="onsiteURL" value="([\p{L}\p{N}\\/\.\?=\#&amp;;\-_~]+|\#(\w)+)"/>
		<regexp name="offsiteURL" value="(\s)*((ht|f)tp(s?)://|mailto:)[\p{L}\p{N}]+[~\p{L}\p{N}\p{Zs}\-_\.@\#\$%&amp;;:,\?=/\+!\(\)]*(\s)*"/>
	</common-regexps>
	<common-attributes>
		<attribute name="lang" description="The 'lang' attribute tells the browser what language the element's attribute values and content are written in">
		 	<regexp-list>
		 		<regexp value="[a-zA-Z]{2,20}"/>
		 	</regexp-list>
		 </attribute>
		 <attribute name="title" description="The 'title' attribute provides text that shows up in a 'tooltip' when a user hovers their mouse over the element">
		 	<regexp-list>
		 		<regexp name="htmlTitle"/>
		 	</regexp-list>
		 </attribute>
		<attribute name="href" onInvalid="filterTag">
			<regexp-list>
				<regexp name="onsiteURL"/>
				<regexp name="offsiteURL"/>
			</regexp-list>
		</attribute>
		<attribute name="align" description="The 'align' attribute of an HTML element is a direction word, like 'left', 'right' or 'center'">
			<literal-list>
				<literal value="center"/>
				<literal value="left"/>
				<literal value="right"/>
				<literal value="justify"/>
				<literal value="char"/>
			</literal-list>
		</attribute>
	</common-attributes>
	<global-tag-attributes>
		<attribute name="title"/>
		<attribute name="lang"/>
	</global-tag-attributes>
	<tags-to-encode>
		<tag>g</tag>
		<tag>grin</tag>
	</tags-to-encode>
	<tag-rules>
		<tag name="script" action="remove"/>
		<tag name="noscript" action="remove"/>
		<tag name="iframe" action="remove"/>
		<tag name="frameset" action="remove"/>
		<tag name="frame" action="remove"/>
		<tag name="noframes" action="remove"/>
		<tag name="style" action="remove"/>
		<tag name="p" action="validate">
			<attribute name="align"/>
		</tag>
		<tag name="div" action="validate"/>		
		<tag name="i" action="validate"/>
		<tag name="b" action="validate"/>
		<tag name="em" action="validate"/>
		<tag name="blockquote" action="validate"/>
		<tag name="tt" action="validate"/>
		<tag name="strong" action="validate"/>
		<tag name="br" action="truncate"/>
		<tag name="quote" action="validate"/>
		<tag name="ecode" action="validate"/> 
		<tag name="a" action="validate">
			<attribute name="href" onInvalid="filterTag"/>
			<attribute name="nohref">
				<literal-list>
					<literal value="nohref"/>
					<literal value=""/>
				</literal-list>
			</attribute>
			<attribute name="rel">
				<literal-list>
					<literal value="nofollow"/>
				</literal-list>
			</attribute>
		</tag>
		<tag name="ul" action="validate"/>
		<tag name="ol" action="validate"/>
		<tag name="li" action="validate"/>
	</tag-rules>
	<css-rules>
	</css-rules>
    <allowed-empty-tags>
        <literal-list>
            <literal value="br"/>
            <literal value="hr"/>
            <literal value="a"/>
            <literal value="img"/>
            <literal value="link"/>
            <literal value="iframe"/>
            <literal value="script"/>
            <literal value="object"/>
            <literal value="applet"/>
            <literal value="frame"/>
            <literal value="base"/>
            <literal value="param"/>
            <literal value="meta"/>
            <literal value="input"/>
            <literal value="textarea"/>
            <literal value="embed"/>
            <literal value="basefont"/>
            <literal value="col"/>
            <literal value="div"/>
        </literal-list>
    </allowed-empty-tags>
</anti-samy-rules>
)XML";

struct RegexMatcher {
    pcre2_code *code = nullptr;
    RegexMatcher() = default;
    RegexMatcher(const std::string &pattern) {
        int errorcode = 0;
        PCRE2_SIZE erroffset = 0;
        std::string anchored = "^(?:" + pattern + ")$";
        code = pcre2_compile(
            (PCRE2_SPTR)anchored.c_str(),
            anchored.length(),
            PCRE2_UTF | PCRE2_UCP | PCRE2_DOTALL,
            &errorcode,
            &erroffset,
            nullptr
        );
    }
    ~RegexMatcher() {
        if (code) pcre2_code_free(code);
    }
    RegexMatcher(const RegexMatcher &o) {
        if (o.code) {
            code = pcre2_code_copy(o.code);
        }
    }
    RegexMatcher& operator=(const RegexMatcher &o) {
        if (this != &o) {
            if (code) pcre2_code_free(code);
            code = o.code ? pcre2_code_copy(o.code) : nullptr;
        }
        return *this;
    }
    RegexMatcher(RegexMatcher &&o) noexcept : code(o.code) {
        o.code = nullptr;
    }
    RegexMatcher& operator=(RegexMatcher &&o) noexcept {
        if (this != &o) {
            if (code) pcre2_code_free(code);
            code = o.code;
            o.code = nullptr;
        }
        return *this;
    }
    bool match(const std::string &subject) const {
        if (!code) return false;
        pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(code, nullptr);
        int rc = pcre2_match(
            code,
            (PCRE2_SPTR)subject.c_str(),
            subject.length(),
            0,
            0,
            match_data,
            nullptr
        );
        pcre2_match_data_free(match_data);
        return rc >= 0;
    }
};

struct AttributeRule {
    std::string name;
    std::set<std::string> allowedLiterals;
    std::vector<RegexMatcher> allowedRegexes;
    bool isValidValue(const std::string &val) const {
        if (allowedLiterals.empty() && allowedRegexes.empty()) return true;
        std::string lowerVal = val;
        for (auto &c : lowerVal) c = tolower((unsigned char)c);
        if (allowedLiterals.count(lowerVal)) return true;
        for (const auto &rx : allowedRegexes) {
            if (rx.match(val)) return true;
        }
        return false;
    }
};

struct TagRule {
    std::string name;
    std::string action; // "validate", "truncate", "remove", "filter"
    std::map<std::string, AttributeRule> attributes;
};

struct Policy {
    size_t maxInputSize = 100000;
    std::map<std::string, TagRule> tags;
    std::map<std::string, AttributeRule> commonAttrs;
    std::map<std::string, AttributeRule> globalAttrs;
    std::map<std::string, std::string> commonRegexps;
};

static Policy parsePolicyXml(const std::string &xmlContent) {
    Policy pol;
    xmlDocPtr doc = xmlReadMemory(xmlContent.c_str(), xmlContent.size(), NULL, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    if (!doc) {
        throw webstrada::exception("Application", "Error validating html input.", "Failed to parse policy XML document.");
    }
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        throw webstrada::exception("Application", "Error validating html input.", "Failed to parse policy XML document.");
    }

    for (xmlNodePtr cur = root->children; cur; cur = cur->next) {
        if (cur->type != XML_ELEMENT_NODE) continue;
        std::string name = (char*)cur->name;
        if (name == "directives") {
            for (xmlNodePtr d = cur->children; d; d = d->next) {
                if (d->type == XML_ELEMENT_NODE && std::string((char*)d->name) == "directive") {
                    xmlChar *dname = xmlGetProp(d, (xmlChar*)"name");
                    xmlChar *dval = xmlGetProp(d, (xmlChar*)"value");
                    if (dname && dval && std::string((char*)dname) == "maxInputSize") {
                        pol.maxInputSize = std::stoull((char*)dval);
                    }
                    if (dname) xmlFree(dname);
                    if (dval) xmlFree(dval);
                }
            }
        } else if (name == "common-regexps") {
            for (xmlNodePtr r = cur->children; r; r = r->next) {
                if (r->type == XML_ELEMENT_NODE && std::string((char*)r->name) == "regexp") {
                    xmlChar *rname = xmlGetProp(r, (xmlChar*)"name");
                    xmlChar *rval = xmlGetProp(r, (xmlChar*)"value");
                    if (rname && rval) {
                        pol.commonRegexps[(char*)rname] = (char*)rval;
                    }
                    if (rname) xmlFree(rname);
                    if (rval) xmlFree(rval);
                }
            }
        } else if (name == "common-attributes") {
            for (xmlNodePtr a = cur->children; a; a = a->next) {
                if (a->type == XML_ELEMENT_NODE && std::string((char*)a->name) == "attribute") {
                    xmlChar *aname = xmlGetProp(a, (xmlChar*)"name");
                    if (!aname) continue;
                    std::string attrName = (char*)aname;
                    for (auto &c : attrName) c = tolower((unsigned char)c);
                    xmlFree(aname);
                    AttributeRule rule;
                    rule.name = attrName;
                    for (xmlNodePtr c = a->children; c; c = c->next) {
                        if (c->type != XML_ELEMENT_NODE) continue;
                        std::string cname = (char*)c->name;
                        if (cname == "literal-list") {
                            for (xmlNodePtr lit = c->children; lit; lit = lit->next) {
                                if (lit->type == XML_ELEMENT_NODE && std::string((char*)lit->name) == "literal") {
                                    xmlChar *lval = xmlGetProp(lit, (xmlChar*)"value");
                                    if (lval) {
                                        std::string s = (char*)lval;
                                        for (auto &ch : s) ch = tolower((unsigned char)ch);
                                        rule.allowedLiterals.insert(s);
                                        xmlFree(lval);
                                    }
                                }
                            }
                        } else if (cname == "regexp-list") {
                            for (xmlNodePtr rx = c->children; rx; rx = rx->next) {
                                if (rx->type == XML_ELEMENT_NODE && std::string((char*)rx->name) == "regexp") {
                                    xmlChar *rxName = xmlGetProp(rx, (xmlChar*)"name");
                                    xmlChar *rxVal = xmlGetProp(rx, (xmlChar*)"value");
                                    std::string pattern;
                                    if (rxName && pol.commonRegexps.count((char*)rxName)) {
                                        pattern = pol.commonRegexps[(char*)rxName];
                                    } else if (rxVal) {
                                        pattern = (char*)rxVal;
                                    }
                                    if (!pattern.empty()) {
                                        rule.allowedRegexes.emplace_back(pattern);
                                    }
                                    if (rxName) xmlFree(rxName);
                                    if (rxVal) xmlFree(rxVal);
                                }
                            }
                        }
                    }
                    pol.commonAttrs[attrName] = std::move(rule);
                }
            }
        } else if (name == "global-tag-attributes") {
            for (xmlNodePtr g = cur->children; g; g = g->next) {
                if (g->type == XML_ELEMENT_NODE && std::string((char*)g->name) == "attribute") {
                    xmlChar *gname = xmlGetProp(g, (xmlChar*)"name");
                    if (gname) {
                        std::string gn = (char*)gname;
                        for (auto &c : gn) c = tolower((unsigned char)c);
                        if (pol.commonAttrs.count(gn)) {
                            pol.globalAttrs[gn] = pol.commonAttrs[gn];
                        } else {
                            AttributeRule r;
                            r.name = gn;
                            pol.globalAttrs[gn] = r;
                        }
                        xmlFree(gname);
                    }
                }
            }
        } else if (name == "tag-rules") {
            for (xmlNodePtr t = cur->children; t; t = t->next) {
                if (t->type == XML_ELEMENT_NODE && std::string((char*)t->name) == "tag") {
                    xmlChar *tname = xmlGetProp(t, (xmlChar*)"name");
                    xmlChar *tact = xmlGetProp(t, (xmlChar*)"action");
                    if (!tname) continue;
                    std::string tn = (char*)tname;
                    for (auto &c : tn) c = tolower((unsigned char)c);
                    xmlFree(tname);
                    TagRule tr;
                    tr.name = tn;
                    tr.action = tact ? (char*)tact : "validate";
                    if (tact) xmlFree(tact);
                    for (xmlNodePtr a = t->children; a; a = a->next) {
                        if (a->type == XML_ELEMENT_NODE && std::string((char*)a->name) == "attribute") {
                            xmlChar *aname = xmlGetProp(a, (xmlChar*)"name");
                            if (!aname) continue;
                            std::string attrName = (char*)aname;
                            for (auto &c : attrName) c = tolower((unsigned char)c);
                            xmlFree(aname);
                            AttributeRule rule;
                            rule.name = attrName;
                            bool hasCustom = false;
                            for (xmlNodePtr c = a->children; c; c = c->next) {
                                if (c->type != XML_ELEMENT_NODE) continue;
                                std::string cname = (char*)c->name;
                                if (cname == "literal-list") {
                                    hasCustom = true;
                                    for (xmlNodePtr lit = c->children; lit; lit = lit->next) {
                                        if (lit->type == XML_ELEMENT_NODE && std::string((char*)lit->name) == "literal") {
                                            xmlChar *lval = xmlGetProp(lit, (xmlChar*)"value");
                                            if (lval) {
                                                std::string s = (char*)lval;
                                                for (auto &ch : s) ch = tolower((unsigned char)ch);
                                                rule.allowedLiterals.insert(s);
                                                xmlFree(lval);
                                            }
                                        }
                                    }
                                } else if (cname == "regexp-list") {
                                    hasCustom = true;
                                    for (xmlNodePtr rx = c->children; rx; rx = rx->next) {
                                        if (rx->type == XML_ELEMENT_NODE && std::string((char*)rx->name) == "regexp") {
                                            xmlChar *rxName = xmlGetProp(rx, (xmlChar*)"name");
                                            xmlChar *rxVal = xmlGetProp(rx, (xmlChar*)"value");
                                            std::string pattern;
                                            if (rxName && pol.commonRegexps.count((char*)rxName)) {
                                                pattern = pol.commonRegexps[(char*)rxName];
                                            } else if (rxVal) {
                                                pattern = (char*)rxVal;
                                            }
                                            if (!pattern.empty()) {
                                                rule.allowedRegexes.emplace_back(pattern);
                                            }
                                            if (rxName) xmlFree(rxName);
                                            if (rxVal) xmlFree(rxVal);
                                        }
                                    }
                                }
                            }
                            if (!hasCustom && pol.commonAttrs.count(attrName)) {
                                rule = pol.commonAttrs[attrName];
                            }
                            tr.attributes[attrName] = std::move(rule);
                        }
                    }
                    pol.tags[tn] = std::move(tr);
                }
            }
        }
    }
    xmlFreeDoc(doc);
    return pol;
}

static std::mutex s_policyMutex;
static std::map<std::string, Policy> s_policyCache;

static const Policy &getPolicy(const std::string &policyPath) {
    std::lock_guard<std::mutex> lock(s_policyMutex);
    if (policyPath.empty()) {
        auto it = s_policyCache.find("<default>");
        if (it == s_policyCache.end()) {
            s_policyCache["<default>"] = parsePolicyXml(DEFAULT_ANTISAMY_POLICY);
        }
        return s_policyCache["<default>"];
    }

    auto it = s_policyCache.find(policyPath);
    if (it != s_policyCache.end()) {
        return it->second;
    }

    std::ifstream file(policyPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw webstrada::exception("Application", "Error validating html input.",
            ("Invalid HTML input does not follow rules in policy xml. Error=java.io.FileNotFoundException: " + policyPath + " (No such file or directory)").c_str());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    s_policyCache[policyPath] = parsePolicyXml(ss.str());
    return s_policyCache[policyPath];
}

static bool ciContainsTag(const std::string &html, const std::string &tag) {
    std::string lowerHtml = html;
    for (auto &c : lowerHtml) c = tolower((unsigned char)c);
    std::string needle = "<" + tag;
    size_t pos = lowerHtml.find(needle);
    while (pos != std::string::npos) {
        size_t nextChar = pos + needle.size();
        if (nextChar >= lowerHtml.size() || isspace((unsigned char)lowerHtml[nextChar]) || lowerHtml[nextChar] == '>' || lowerHtml[nextChar] == '/') {
            return true;
        }
        pos = lowerHtml.find(needle, pos + 1);
    }
    return false;
}

static bool validateNode(xmlNodePtr node, const Policy &policy, bool hasHtmlTagInInput, bool hasBodyTagInInput) {
    for (xmlNodePtr cur = node; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            std::string tag = (char*)cur->name;
            for (auto &c : tag) c = tolower((unsigned char)c);
            if (tag == "html" && !hasHtmlTagInInput) {
                if (!validateNode(cur->children, policy, hasHtmlTagInInput, hasBodyTagInInput)) return false;
                continue;
            }
            if (tag == "body" && !hasBodyTagInInput) {
                if (!validateNode(cur->children, policy, hasHtmlTagInInput, hasBodyTagInInput)) return false;
                continue;
            }
            auto it = policy.tags.find(tag);
            if (it == policy.tags.end()) {
                return false;
            }
            const TagRule &tr = it->second;
            if (tr.action != "validate" && tr.action != "truncate") {
                return false;
            }
            if (tr.action == "truncate") {
                if (cur->properties != nullptr) return false;
                for (xmlNodePtr c = cur->children; c; c = c->next) {
                    if (c->type != XML_TEXT_NODE) return false;
                }
            } else {
                for (xmlAttrPtr a = cur->properties; a; a = a->next) {
                    std::string aname = (char*)a->name;
                    for (auto &c : aname) c = tolower((unsigned char)c);
                    const AttributeRule *arule = nullptr;
                    auto ait = tr.attributes.find(aname);
                    if (ait != tr.attributes.end()) {
                        arule = &ait->second;
                    } else {
                        auto git = policy.globalAttrs.find(aname);
                        if (git != policy.globalAttrs.end()) {
                            arule = &git->second;
                        }
                    }
                    if (!arule) return false;
                    xmlChar *val = xmlGetProp(cur, a->name);
                    std::string sval = val ? (char*)val : "";
                    if (val) xmlFree(val);
                    if (!arule->isValidValue(sval)) return false;
                }
            }
            if (!validateNode(cur->children, policy, hasHtmlTagInInput, hasBodyTagInInput)) return false;
        }
    }
    return true;
}

} // namespace

namespace cfml {

cfvariant *cf_issafehtml(const cfvariant *input, const cfvariant *policyFile) {
    if (!input) {
        throw webstrada::exception("The function accepts 1 to 2 parameters.");
    }
    if (input->m_type == cfvariant::Struct || input->m_type == cfvariant::Array ||
        input->m_type == cfvariant::Query || input->m_type == cfvariant::Xml) {
        throw webstrada::exception("Complex object types cannot be converted to simple values.");
    }

    std::string resolvedPolicyPath;
    if (policyFile && policyFile->m_type != cfvariant::Null) {
        string pfStr = const_cast<cfvariant*>(policyFile)->toString().trimmed();
        if (!pfStr.isEmpty()) {
            std::filesystem::path p(pfStr.constData());
            if (!p.is_absolute()) {
                cfml::IncludeRuntime *inc = cfml::include_context();
                if (inc && !inc->currentPath.empty()) {
                    std::filesystem::path curDir = std::filesystem::path(inc->currentPath).parent_path();
                    std::filesystem::path resolved = curDir / p;
                    if (std::filesystem::exists(resolved)) {
                        p = resolved;
                    } else if (!inc->webRoot.empty()) {
                        std::filesystem::path rootResolved = std::filesystem::path(inc->webRoot) / p;
                        if (std::filesystem::exists(rootResolved)) {
                            p = rootResolved;
                        }
                    }
                }
            }
            resolvedPolicyPath = p.string();
        }
    }

    const Policy &policy = getPolicy(resolvedPolicyPath);

    string htmlStr = const_cast<cfvariant*>(input)->toString();
    if (htmlStr.isEmpty()) {
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = true;
        return ret;
    }
    if (htmlStr.length() > (int)policy.maxInputSize) {
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = false;
        return ret;
    }

    std::string htmlStd(htmlStr.constData(), htmlStr.length());
    htmlDocPtr doc = htmlReadMemory(htmlStd.data(), htmlStd.size(), NULL, "UTF-8", HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    if (!doc) {
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = false;
        return ret;
    }

    bool hasHtml = ciContainsTag(htmlStd, "html");
    bool hasBody = ciContainsTag(htmlStd, "body");
    xmlNodePtr root = xmlDocGetRootElement(doc);
    bool ok = validateNode(root, policy, hasHtml, hasBody);
    xmlFreeDoc(doc);

    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = ok;
    return ret;
}

} // namespace cfml

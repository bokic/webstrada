/**
 * @file fn_getmetadata.cpp
 * @brief CFML getmetadata() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/component.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <string>
#include <vector>

namespace cfml {

cfvariant *cf_getmetadata(const cfvariant *obj) {
    if (!obj) throw webstrada::exception("GetMetaData requires exactly 1 argument");

    // Component: CF's GetMetaData returns the component metadata struct with
    // upper-case keys (PATH, EXTENDS, PROPERTIES, FUNCTIONS, TYPE, FULLNAME,
    // NAME) — a different key set from GetComponentMetaData (which uses
    // lowercase keys). Verified against CF 2025 on the RDS host.
    if (obj->m_type == cfvariant::Component && obj->m_component && obj->m_component->info) {
        ComponentInfo *info = obj->m_component->info;
        // NAME/FULLNAME: the dot path used to instantiate (e.g. "components.meta"),
        // derived from the .cfc path relative to the web root when known.
        std::string dotName = info->path;
        {
            IncludeRuntime *rt = cfml::include_context();
            if (rt && !rt->webRoot.empty() && !info->cfcPath.empty()) {
                std::filesystem::path root(rt->webRoot);
                std::filesystem::path cfc(info->cfcPath);
                std::error_code ec;
                std::string rel = std::filesystem::relative(cfc, root, ec).generic_string();
                if (!ec && !rel.empty()) {
                    if (rel.size() >= 4 && rel.compare(rel.size() - 4, 4, ".cfc") == 0)
                        rel = rel.substr(0, rel.size() - 4);
                    for (auto &c : rel) if (c == '/') c = '.';
                    dotName = rel;
                }
            }
        }
        cfvariant *md = new cfvariant(cfvariant::Struct);
        md->structSet("PATH", cfvariant(info->cfcPath.c_str()));

        // EXTENDS: parent metadata struct (or an empty struct for no parent).
        cfvariant ext(cfvariant::Struct);
        if (info->parent) {
            ext.structSet("PATH", cfvariant(info->parent->cfcPath.c_str()));
            ext.structSet("TYPE", cfvariant("component"));
            ext.structSet("FULLNAME", cfvariant(info->parent->path.c_str()));
            ext.structSet("NAME", cfvariant(info->parent->path.c_str()));
        }
        md->structSet("EXTENDS", ext);

        // PROPERTIES: array of {TYPE, NAME}.
        cfvariant props(cfvariant::Array);
        for (const auto &p : info->properties) {
            cfvariant pr(cfvariant::Struct);
            pr.structSet("TYPE", cfvariant(p.type.c_str()));
            pr.structSet("NAME", cfvariant(p.name.c_str()));
            props.insert(pr);
        }
        md->structSet("PROPERTIES", props);

        // FUNCTIONS: array of {NAME, PARAMETERS}.
        cfvariant funcs(cfvariant::Array);
        for (const auto &m : info->methods) {
            cfvariant f(cfvariant::Struct);
            f.structSet("NAME", cfvariant(m.declaredName.empty() ? m.name.c_str() : m.declaredName.c_str()));
            cfvariant params(cfvariant::Array);
            for (const auto &p : m.params) {
                cfvariant prm(cfvariant::Struct);
                prm.structSet("name", p.name);
                prm.structSet("type", p.type);
                cfvariant req(cfvariant::Boolean);
                req.m_bool = false;
                req.m_boolLiteral = true; // CF renders the literal `false`
                prm.structSet("required", req);
                prm.structSet("default", cfvariant(p.defaultValue));
                params.insert(prm);
            }
            f.structSet("PARAMETERS", params);
            funcs.insert(f);
        }
        md->structSet("FUNCTIONS", funcs);

        md->structSet("TYPE", cfvariant("component"));
        md->structSet("FULLNAME", cfvariant(dotName.c_str()));
        md->structSet("NAME", cfvariant(dotName.c_str()));
        return md;
    }

    // User-defined function: {NAME, PARAMETERS:[{name,type,required,default}]}.
    if (obj->m_type == cfvariant::Function && obj->m_udf) {
        UDFInfo *udf = obj->m_udf;
        cfvariant *md = new cfvariant(cfvariant::Struct);
        md->structSet("NAME", cfvariant(udf->name.isEmpty() ? "anonymous" : udf->name));
        cfvariant params(cfvariant::Array);
        for (const auto &p : udf->params) {
            cfvariant prm(cfvariant::Struct);
            prm.structSet("name", p.name);
            prm.structSet("type", p.type);
            cfvariant req(cfvariant::Boolean);
            req.m_bool = false;
            req.m_boolLiteral = true; // CF renders the literal `false`
            prm.structSet("required", req);
            prm.structSet("default", cfvariant(p.defaultValue));
            params.insert(prm);
        }
        md->structSet("PARAMETERS", params);
        return md;
    }

    // Queries and simple values: CF returns Java objects (QueryTableMetaData /
    // Class) that this engine cannot replicate. Per the action plan, throw a
    // runtime exception for the unsupported cases.
    throw webstrada::exception("GetMetaData",
                              "GetMetaData is only supported for components and user-defined functions in this engine.",
                              "ColdFusion returns Java objects for queries and simple values; those cannot be replicated.");
}

} // namespace cfml

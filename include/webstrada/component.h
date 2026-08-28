#pragma once

#include "cfvariant.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace webstrada {

// A ColdFusion Component (CFC). A component definition (ComponentInfo) is the
// compiled, shared "class": the component name/path, its properties, its method
// table and the parent (extends) chain. A component instance (ComponentInstance)
// is one object: an owned `this` scope (public members, also exposed through the
// cfvariant's StructData so struct introspection/SerializeJSON/access work on
// it) plus an owned `variables` scope (private instance variables). A cfvariant
// of type Component holds a retained ComponentInstance in m_component and its
// this-scope StructData in m_structData (the instance owns the StructData).

struct ComponentMethod {
    std::string name;            // upper-cased (lookups are case-insensitive)
    std::string declaredName;    // as declared (e.g. "getX") for introspection
    std::string access;          // public / private / package / remote
    std::string returnType;      // "" -> any
    void *fn = nullptr;          // JIT cfc-method entry (component_method_entry_fn)
    std::vector<UdfParamInfo> params;   // introspection (cfdump/GetComponentMetaData)
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::vector<bool> paramRequired;    // explicit required= attribute per param
};

struct ComponentProperty {
    std::string name;
    std::string type;
    std::string defaultText;     // raw attribute string
    std::string access;
};

// Compiled component definition, shared by every instance. Owned by the
// compiled-component cache (which keeps the JIT module alive). Refcounted so an
// instance can outlive the cache entry. Also used for ColdFusion interfaces
// (a .cfc whose top level is <cfinterface>/`interface`): those carry isInterface
// and their method table holds the declared signatures (bodies are not
// compiled); `interfaceParents` are the resolved `extends` interfaces.
struct ComponentInfo {
    int refs = 1;
    std::string name;            // component name (file base name, e.g. "comp1")
    std::string path;            // dot path used to instantiate (e.g. "comp1", "foo.bar")
    std::string cfcPath;         // absolute file system path of the .cfc
    std::string fullName;        // web-root-relative dot path (e.g. "tmp_iface.comp1")
    std::string displayPath;     // the path used to load it (createObject/new path verbatim)
    std::string extendsPath;     // raw `extends` attribute ("" when none)
    ComponentInfo *parent = nullptr;  // resolved parent (retained), null when none
    std::vector<ComponentMethod> methods;
    std::unordered_map<std::string, int> methodMap;
    std::vector<ComponentProperty> properties;
    // Construction body entry: runs the component's top-level statements
    // (this.x = ..., variables.y = ..., property defaults) with the instance's
    // scopes. Signature component_body_fn.
    void *body = nullptr;

    // Interface support.
    bool isInterface = false;          // this .cfc is a <cfinterface>/interface
    std::string displayName;           // interface displayname attribute
    std::string hint;                  // interface hint attribute
    std::vector<std::string> extendsList;   // interface `extends` values (comma list)
    std::vector<ComponentInfo*> interfaceParents; // resolved extends interfaces (retained)
    std::string implementsText;        // raw `implements` attribute (comma list)
    std::vector<ComponentInfo*> interfaces;     // resolved implemented interfaces (retained)
    bool validationDone = false;       // interface validation ran
};

// One live component object.
struct ComponentInstance {
    int refs = 1;
    ComponentInfo *info = nullptr;        // retained
    cfvariant *thisScope = nullptr;       // owned Struct (the cfvariant's m_structData)
    cfvariant *variablesScope = nullptr;  // owned Struct
};

// Runtime helpers (implemented in src/core/core_component.cpp).
ComponentInfo *component_info_retain(ComponentInfo *info);
void component_info_release(ComponentInfo *info);
ComponentInstance *component_instance_retain(ComponentInstance *inst);
void component_instance_release(ComponentInstance *inst);

} // namespace webstrada

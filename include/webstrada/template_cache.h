#pragma once

#include "llvm_codegen.h"
#include "string.h"

#include <map>
#include <ctime>


namespace webstrada {

class TemplateCache {
public:
    template_fn get(const string &pathname);

    // Returns a retained ComponentInfo for the .cfc at `pathname` (compiling
    // it on first access), or nullptr when the file does not exist. The
    // returned ComponentInfo is owned by the caller (component_info_release).
    // Definitions are cached per path; recompiles when the file changes.
    ComponentInfo *get_component(const string &pathname);

    // Clears all compiled templates and components from memory.
    void clear();

    ~TemplateCache();

private:
    time_t getFileModTime(const string &pathname);
    std::map<string, template_fn> m_templates;
    std::map<string, time_t> m_timestamps;
    std::map<string, ComponentInfo*> m_components;
    std::map<string, time_t> m_componentTimestamps;
    llvm_codegen m_codegen;
};

// Process-global counts of JIT-compiled templates (.cfm) and components (.cfc)
// currently held by the worker's TemplateCache. Each prefork worker is a
// separate process, so these are per-worker like the request stats; surfaced
// through __cacheInfo() for the admin Cache page.
void compiled_cache_counts(int &templates, int &components);

}


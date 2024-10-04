#include <webstrada/template_cache.h>
#include <webstrada/component.h>
#include <webstrada/worker.h>

#include <sys/stat.h>


using namespace webstrada;

// Live counts of JIT-compiled templates/components in this worker process
// (see webstrada/template_cache.h). Maintained on insert/erase in get() and
// get_component().
static int g_compiledTemplates = 0;
static int g_compiledComponents = 0;

void webstrada::compiled_cache_counts(int &templates, int &components)
{
    templates = g_compiledTemplates;
    components = g_compiledComponents;
}

TemplateCache::~TemplateCache()
{
    // The cached component definitions are owned by the cache (get_component
    // retains before returning); release them so the code they point into
    // (m_codegen's engines) is not left dangling/leaked at cache destruction.
    for (auto &kv : m_components) component_info_release(kv.second);
    m_components.clear();
}

time_t TemplateCache::getFileModTime(const string &pathname)
{
    struct stat result;
    if (stat(pathname.constData(), &result) == 0)
        return result.st_mtime;
    return 0;
}

template_fn TemplateCache::get(const string &pathname)
{
    template_fn ret = nullptr;

#ifndef ALWAYS_RECOMPILE_CFTEMPLATES
    if (m_templates.contains(pathname))
    {
        time_t current_mod = getFileModTime(pathname);
        time_t cached_mod = m_timestamps.at(pathname);
        if (current_mod == cached_mod)
        {
            return m_templates.at(pathname);
        }
        m_templates.erase(pathname);
        m_timestamps.erase(pathname);
        g_compiledTemplates--;
    }
#endif

    ret = m_codegen.compile(pathname);

#ifndef ALWAYS_RECOMPILE_CFTEMPLATES
    m_templates.insert({pathname, ret});
    m_timestamps.insert({pathname, getFileModTime(pathname)});
    g_compiledTemplates++;
#endif

    return ret;
}

ComponentInfo *TemplateCache::get_component(const string &pathname)
{
    if (getFileModTime(pathname) == 0) return nullptr;
    auto it = m_components.find(pathname);
    if (it != m_components.end()) {
        return component_info_retain(it->second);
    }
    ComponentInfo *info = m_codegen.compileComponent(pathname);
    auto cit = m_components.find(pathname);
    bool isNewEntry = (cit == m_components.end());
    if (cit != m_components.end()) component_info_release(cit->second);
    m_components[pathname] = info;
    if (isNewEntry) g_compiledComponents++;
    return component_info_retain(info);
}

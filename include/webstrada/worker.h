#pragma once

#include "template_cache.h"
#include "cfvariant.h"
#include "exceptions.h"
#include "scope_store.h"


struct FCGX_Request;

namespace webstrada
{

class worker
{
public:
    friend class WorkerTest;
    worker();
    void process_request(FCGX_Request *request);
    void process_cli_request(const string &pathname, const string &web_root);

    string &out();
    cfvariant &cgi();
    cfvariant &server();
    cfvariant &cookie();
    cfvariant &application();
    cfvariant &session();
    cfvariant &url();
    cfvariant &form();
    cfvariant &variables();

private:
    void writeException(const webstrada::exception &ex);
    void run_template(const string &pathname, const string &web_root);
    string find_application_cfm(const string &template_path, const string &web_root);
    string find_application_cfc(const string &template_path, const string &web_root);
    // Runs the whole request through the nearest Application.cfc lifecycle
    // (onApplicationStart/onSessionStart/onRequestStart/onRequest/onRequestEnd/
    // onMissingTemplate/onError). Returns true when the CFC handled the request
    // (or the request was aborted); false when no Application.cfc applies.
    bool run_application_cfc(const string &app_cfc_path, const string &pathname,
                             const string &web_root);
    void open_scope_store();

    string m_out;
    cfvariant m_cgi = cfvariant::Struct;         // read only scope
    cfvariant m_server = cfvariant::Struct;      // read only scope
    cfvariant m_cookie = cfvariant::Struct;
    cfvariant m_application = cfvariant::Struct; // can have disabled state
    cfvariant m_session = cfvariant::Struct;     // can have disabled state
    cfvariant m_url = cfvariant::Struct;         // read only scope
    cfvariant m_form = cfvariant::Struct;        // read only scope
    cfvariant m_variables = cfvariant::Struct;
    TemplateCache m_templates;
    ScopeStore m_scopeStore;                     // SQLite-backed APPLICATION/SESSION scopes

    // Cached Application.cfc instance (persists across requests in this worker,
    // matching ColdFusion's per-JVM application object).
    cfvariant m_appCfc = cfvariant::Struct;
    string m_appCfcPath;
    bool m_appCfcStarted = false;
};

}

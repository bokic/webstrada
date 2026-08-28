#include <webstrada/worker.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/cf8.h>
#include <webstrada/component.h>
#include <webstrada/upload.h>
#include <webstrada/config.h>
#include <webstrada/cache_store.h>
#include <webstrada/server_stats.h>
#include <webstrada/db.h>
#include "cftags/common.h"
#include "core/core_internal.h"

#include <functional>
#include <cstring>
#include <format>
#include <ctime>
#include <chrono>

#include <fcgio.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>


using namespace webstrada;

// The FastCGI output stream for the request currently being processed. The
// response-state writer routes every flushed byte through it; it is thread-local
// because forked workers each run their own request loop.
static thread_local FCGX_Stream *g_response_stream = nullptr;

static void fcgiResponseWrite(const char *data, size_t len)
{
    if (g_response_stream && len > 0) {
        FCGX_PutStr(data, len, g_response_stream);
    }
}

// Loads the compiled template for an absolute path from the worker's
// compiled-template cache (the `opaque` pointer). Returns nullptr when the file
// does not exist, so <cfinclude> can raise CF's template-not-found error.
static cfml::include_template_fn include_template_loader(const char *path, void *opaque)
{
    TemplateCache *cache = static_cast<TemplateCache*>(opaque);
    if (!cache) return nullptr;
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return nullptr;
    }
    template_fn fn = cache->get(webstrada::string(path));
    auto *target = fn.target<cfml::include_template_fn>();
    return target ? *target : nullptr;
}

// Loads the compiled ColdFusion component for an absolute .cfc path from the
// worker's compiled-component cache (the `opaque` pointer).
static webstrada::ComponentInfo *component_loader(const char *path, void *opaque)
{
    TemplateCache *cache = static_cast<TemplateCache*>(opaque);
    if (!cache) return nullptr;
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return nullptr;
    }
    return cache->get_component(webstrada::string(path));
}

static thread_local worker *g_currentWorker = nullptr;

static void worker_cache_invalidator()
{
    if (g_currentWorker) {
        g_currentWorker->clear_compiled_caches();
    }
    cfml::custom_tag_target_cache_clear();
    webstrada::db::closeAllConnections();
}

worker::worker() {
    g_currentWorker = this;
    webstrada::config::setCacheInvalidator(worker_cache_invalidator);

    // Fill SERVER scope
    cfml::init_server_scope(m_server);

    open_scope_store();
    open_cache_store();
    open_profiler_store();
}

worker::~worker() {
    webstrada::db::closeAllConnections();
}

void worker::open_profiler_store()
{
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    std::string dbPath;
    if (n > 0) {
        exe[n] = '\0';
        std::string path(exe);
        size_t slash = path.find_last_of('/');
        dbPath = (slash != std::string::npos)
            ? path.substr(0, slash + 1) + "WebStrada-profiler.sqlite"
            : "WebStrada-profiler.sqlite";
    } else {
        dbPath = "WebStrada-profiler.sqlite";
    }
    if (!m_profilerStore.open(dbPath)) {
        fprintf(stderr, "[WebStrada] Warning: could not open profiler database %s: %s\n",
                dbPath.c_str(), m_profilerStore.lastError().c_str());
    }
}

// Resolve the SQLite scope database path and open it. The default location is
// next to the WebStrada binary (config::scopeDbPath overrides). A failure to
// open is non-fatal: scopes simply stay unavailable for the worker's life (the
// <cfapplication> helpers throw a clear error when asked to enable them).
void worker::open_scope_store()
{
    std::string dbPath = webstrada::config::scopeDbPath;
    if (dbPath.empty()) {
        char exe[4096];
        ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (n > 0) {
            exe[n] = '\0';
            std::string path(exe);
            size_t slash = path.find_last_of('/');
            dbPath = (slash != std::string::npos)
                ? path.substr(0, slash + 1) + "WebStrada-scopes.sqlite"
                : "WebStrada-scopes.sqlite";
        } else {
            dbPath = "WebStrada-scopes.sqlite";
        }
    }
    if (!m_scopeStore.open(dbPath)) {
        fprintf(stderr, "[WebStrada] Warning: could not open scope database %s: %s\n",
                dbPath.c_str(), m_scopeStore.lastError().c_str());
    }
}

void worker::process_request(FCGX_Request *request)
{
    auto t_req_start = std::chrono::steady_clock::now();
    try {
        g_currentWorker = this;
        cfml::trace_begin_request(t_req_start);
        cfml::trace_record_event("ENGINE", "[ENGINE]", "REQUEST_ACCEPT", 0);

        // Re-read the server config file when it changed on disk, so an
        // admin-panel update (written by any worker) becomes effective on the
        // next request in this prefork child (a single stat() when unchanged).
        if (webstrada::config::reloadIfChanged()) {
            m_templates.clear();
            cfml::custom_tag_target_cache_clear();
            webstrada::db::closeAllConnections();
        }

        g_reqProfiler.reset();

        // Reset the per-request HTTP request body (GetHttpRequestData reads it).
        cfml::request_set_body(nullptr, 0);
        // Fill CGI scope.
        m_cgi.setAutoCreate();
        for(int c = 0; request->envp[c]; c++) {
            char *item = request->envp[c];

            const char *separator = strstr(item, "=");
            if (separator) {
                size_t strsize = separator - item;
                if (strsize > 0) {
                    string key(item, strsize);
                    const char *value = item + strsize + 1;

                    key.toUpper();
                    m_cgi[key] = value;
                }
            }
        }

        // Normalize SCRIPT_NAME / PATH_INFO / REQUEST_URI to always have a leading slash and never be empty.
        if (m_cgi.has("SCRIPT_NAME") && !m_cgi["SCRIPT_NAME"].toString().isEmpty()) {
            string sn = m_cgi["SCRIPT_NAME"];
            if (!sn.startWith("/")) {
                m_cgi["SCRIPT_NAME"] = "/" + sn;
            }
        } else if (m_cgi.has("REQUEST_URI") && !m_cgi["REQUEST_URI"].toString().isEmpty()) {
            string ru = m_cgi["REQUEST_URI"];
            if (ru.contains('?')) {
                ru = ru.left(ru.indexOf('?'));
            }
            if (!ru.startWith("/")) {
                ru = "/" + ru;
            }
            m_cgi["SCRIPT_NAME"] = ru;
        } else {
            m_cgi["SCRIPT_NAME"] = "/";
        }

        if (m_cgi.has("PATH_INFO")) {
            string pi = m_cgi["PATH_INFO"];
            if (!pi.isEmpty() && !pi.startWith("/")) {
                m_cgi["PATH_INFO"] = "/" + pi;
            }
        } else {
            m_cgi["PATH_INFO"] = "";
        }

        if (m_cgi.has("REQUEST_URI")) {
            string ru = m_cgi["REQUEST_URI"];
            if (!ru.isEmpty() && !ru.startWith("/")) {
                m_cgi["REQUEST_URI"] = "/" + ru;
            }
        } else {
            m_cgi["REQUEST_URI"] = m_cgi["SCRIPT_NAME"];
        }
        m_cgi.setReadOnly();

        // Record the request start for the admin dashboard stats (duration,
        // uptime, request count, recent-request list).
        webstrada::stats::request_begin(m_cgi["REQUEST_METHOD"].toString().constData(),
                                        m_cgi["REQUEST_URI"].toString().constData());

        // Fill URL scope
        m_url.setAutoCreate();
        string query_string = m_cgi["QUERY_STRING"];
        if (!query_string.isEmpty()) {
            auto items = query_string.split('&', false);
            for(const auto &item: items) {
                const auto &key_value = item.split('=', true);
                const auto &key = key_value.at(0).percent_decode();
                const auto &value = key_value.at(1).percent_decode();
                m_url[key] = value;
            }
        }
        m_url.setReadOnly();

        // TODO: Fill FORM scope
        m_form.setAutoCreate();
        string request_method = m_cgi["REQUEST_METHOD"];
        if (request_method.equals("POST")) {
            if ((m_cgi.has("CONTENT_LENGTH"))&&(m_cgi.has("CONTENT_TYPE"))) {
                string content_type = m_cgi["CONTENT_TYPE"];
                int content_length = m_cgi["CONTENT_LENGTH"].toString().toInt();

                string form;
                form.resize(content_length);
                FCGX_GetStr(form.data(), content_length, request->in);
                cfml::request_set_body(form.constData(), form.length());

                if (content_type.startWith("application/x-www-form-urlencoded")) {
                    const auto &pairs = form.split('&');
                    cfvariant keys = cfvariant::Array;
                    for(const auto &pair: pairs) {
                        const auto &key_value = pair.split('=');
                        if (key_value.size() == 2) {
                            auto key = key_value.at(0).percent_decode();
                            const auto &tt = key_value.at(1);
                            auto value = key_value.at(1).percent_decode();
                            key.toUpper();
                            keys.insert(key);
                            m_form[key] = value;
                        }
                    }

                    m_form["FIELDNAMES"] = keys.join(',');
                    m_form["form"] = form;
                }
                else if (content_type.startWith("multipart/form-data")) {
                    // Extract the boundary parameter from Content-Type.
                    string boundary;
                    int bpos = content_type.indexOf("boundary=");
                    if (bpos >= 0) {
                        boundary = content_type.mid(bpos + 9, content_type.length() - bpos - 9);
                        if (!boundary.isEmpty() && boundary.at(0) == '"') boundary = boundary.mid(1, boundary.length() - 1);
                        if (!boundary.isEmpty() && boundary.at(boundary.length() - 1) == '"') boundary = boundary.left(boundary.length() - 1);
                    }

                    std::string body(form.constData(), form.length());
                    std::vector<webstrada::UploadedFile> uploads;
                    cfvariant keys = cfvariant::Array;

                    if (!boundary.isEmpty()) {
                        std::string delim = "--" + std::string(boundary.constData(), boundary.length());
                        size_t pos = 0;
                        while (true) {
                            size_t start = body.find(delim, pos);
                            if (start == std::string::npos) break;
                            start += delim.size();
                            // Skip the CRLF (or LF) after the boundary marker.
                            if (start < body.size() && body[start] == '\r') start++;
                            if (start < body.size() && body[start] == '\n') start++;

                            size_t end = body.find(delim, start);
                            if (end == std::string::npos) break;
                            pos = end;

                            std::string part = body.substr(start, end - start);
                            if (!part.empty() && part.back() == '\n') part.pop_back();
                            if (!part.empty() && part.back() == '\r') part.pop_back();
                            if (part.empty()) continue;

                            // Split the part headers from the part content.
                            size_t hdrEnd = part.find("\r\n\r\n");
                            size_t hdrLen = 4;
                            if (hdrEnd == std::string::npos) {
                                hdrEnd = part.find("\n\n");
                                hdrLen = 2;
                            }
                            if (hdrEnd == std::string::npos) continue;

                            std::string headers = part.substr(0, hdrEnd);
                            std::string content = part.substr(hdrEnd + hdrLen);

                            // Parse Content-Disposition: form-data; name="..."; filename="..."
                            std::string fieldName, filename, partContentType;
                            size_t cd = headers.find("Content-Disposition:");
                            if (cd != std::string::npos) {
                                std::string cdLine = headers.substr(cd);
                                size_t nq = cdLine.find("name=\"");
                                if (nq != std::string::npos) {
                                    nq += 6;
                                    size_t nqe = cdLine.find('"', nq);
                                    if (nqe != std::string::npos) fieldName = cdLine.substr(nq, nqe - nq);
                                }
                                size_t fq = cdLine.find("filename=\"");
                                if (fq != std::string::npos) {
                                    fq += 10;
                                    size_t fqe = cdLine.find('"', fq);
                                    if (fqe != std::string::npos) filename = cdLine.substr(fq, fqe - fq);
                                }
                            }
                            size_t ct = headers.find("Content-Type:");
                            if (ct != std::string::npos) {
                                std::string ctLine = headers.substr(ct + 13);
                                size_t semi = ctLine.find(';');
                                if (semi != std::string::npos) ctLine = ctLine.substr(0, semi);
                                while (!ctLine.empty() && (ctLine.front() == ' ' || ctLine.front() == '\r' || ctLine.front() == '\n')) ctLine.erase(ctLine.begin());
                                while (!ctLine.empty() && (ctLine.back() == ' ' || ctLine.back() == '\r' || ctLine.back() == '\n')) ctLine.pop_back();
                                partContentType = ctLine;
                            }

                            if (fieldName.empty()) continue;

                            string fName(fieldName.c_str());
                            fName.toUpper();

                            if (!filename.empty()) {
                                // File upload part: keep only the basename.
                                size_t slash = filename.find_last_of("/\\");
                                std::string baseFile = (slash == std::string::npos) ? filename : filename.substr(slash + 1);

                                webstrada::UploadedFile uf;
                                uf.fieldName = fieldName;
                                uf.filename = baseFile;
                                uf.contentType = partContentType;
                                uf.content.assign(reinterpret_cast<const std::byte*>(content.data()),
                                                  reinterpret_cast<const std::byte*>(content.data()) + content.size());
                                uploads.push_back(std::move(uf));

                                // FORM[fieldName] = {CLIENTFILE, CLIENTFILENAME, CLIENTFILEEXT}
                                webstrada::string base(baseFile.c_str());
                                webstrada::string ext;
                                int dot = base.lastIndexOf('.');
                                if (dot > 0 && dot < base.length() - 1) ext = base.mid(dot + 1, base.length() - dot - 1);
                                webstrada::string baseName = (dot > 0) ? base.left(dot) : base;

                                cfvariant uploadStruct = cfvariant::Struct;
                                uploadStruct.set("CLIENTFILE") = base;
                                uploadStruct.set("CLIENTFILENAME") = baseName;
                                uploadStruct.set("CLIENTFILEEXT") = ext;
                                m_form[fName] = uploadStruct;
                                keys.insert(fName);
                            } else {
                                // Regular form field.
                                string value(content.c_str());
                                m_form[fName] = value;
                                keys.insert(fName);
                            }
                        }
                    }

                    m_form["FIELDNAMES"] = keys.join(',');
                    m_form["MULTIPART_FORM_DATA"] = "true";
                    webstrada::UploadRegistry::instance().setFiles(std::move(uploads));
                }
            }
        }
        m_form.setReadOnly();

        // Fill COOKIE scope from the Cookie header. ColdFusion exposes cookies
        // case-insensitively; the keys are stored uppercased like the other
        // read-only request scopes. Sessions read CFID/CFTOKEN from here.
        m_cookie.setAutoCreate();
        string cookie_header = m_cgi["HTTP_COOKIE"];
        if (!cookie_header.isEmpty()) {
            auto items = cookie_header.split(';');
            for (const auto &item : items) {
                string kv = item.trimmed();
                int eq = kv.indexOf('=');
                if (eq > 0) {
                    string key = kv.left(eq).trimmed();
                    string value = kv.mid(eq + 1, kv.length() - eq - 1).trimmed();
                    key.toUpper();
                    m_cookie[key] = value;
                }
            }
        }
        m_cookie.setReadOnly();

        m_application.setDisabled();
        m_session.setDisabled();

        cfml::trace_record_event("ENGINE", "[ENGINE]", "REQUEST_SCOPES_INIT");

        // Bind the SQLite-backed APPLICATION/SESSION scopes for this request.
        // scope_end() persists them back (RAII so it also runs on exceptions).
        cfml::scope_begin(&m_scopeStore, &m_application, &m_session);
        cfml::trace_record_event("ENGINE", "[ENGINE]", "SCOPE_STORE_BINDING");
        struct ScopeSaveGuard {
            ~ScopeSaveGuard() { cfml::scope_end(); }
        } scopeSaveGuard;

        string pathname = m_cgi["DOCUMENT_ROOT"].toString() + m_cgi["REQUEST_URI"].toString();

        // if pathname has ? then it's a query string, strip it
        if (pathname.contains('?')) {
            pathname = pathname.left(pathname.indexOf('?'));
        }

        // Bind the web-engine output stream and reset the per-request response
        // state (output charset from the server config, Content-Type default,
        // not yet committed). <cfcontent> and <cfflush> update this state and
        // may write bytes to the stream mid-request.
        g_response_stream = request->out;
        cfml::response_set_write_fn(fcgiResponseWrite);
        cfml::response_begin();
        cfml::response_set_stream(request->out);

        cfml::cferror_reset();

        // The request's web path (error.template value): the filesystem path
        // with the document root stripped.
        std::string requestWebPath = pathname.constData() ? std::string(pathname.constData(), pathname.length()) : std::string();
        {
            std::string rootStr(m_cgi["DOCUMENT_ROOT"].toString().constData());
            while (rootStr.size() > 1 && rootStr.back() == '/') rootStr.pop_back();
            if (!rootStr.empty() && requestWebPath.compare(0, rootStr.size(), rootStr) == 0) {
                requestWebPath = requestWebPath.substr(rootStr.size());
            }
            if (requestWebPath.empty() || requestWebPath[0] != '/') requestWebPath = "/" + requestWebPath;
        }

        cfml::VariantCleanupGuard guard;

        // Set up the <cfinclude> runtime context for this request. The loader
        // hands back compiled templates from this worker's cache; relative
        // includes resolve against the template currently executing (updated
        // in run_template), absolute (/...) ones against the web root.
        cfml::IncludeRuntime includeRuntime;
        includeRuntime.webRoot = std::string(m_cgi["DOCUMENT_ROOT"].toString().constData());
        includeRuntime.loader = &include_template_loader;
        includeRuntime.loaderOpaque = &m_templates;
        includeRuntime.componentLoader = &component_loader;
        includeRuntime.componentLoaderOpaque = &m_templates;
        cfml::include_begin(&includeRuntime);
        cfml::trace_record_event("ENGINE", "[ENGINE]", "INCLUDE_RUNTIME_INIT");
        struct IncludeGuard {
            ~IncludeGuard() { cfml::include_end(); }
        } includeGuard;

        try {
            run_template(pathname, m_cgi["DOCUMENT_ROOT"].toString());
        } catch(const webstrada::abort_exception &ex) {
        } catch(const webstrada::exit_exception &ex) {
        } catch(const webstrada::template_exception &ex) {
            if (!cfml::cf_cferror_handle(&ex, &m_out, &m_cgi, &m_server, &m_cookie,
                                      &m_application, &m_session, &m_url, &m_form,
                                      &m_variables, requestWebPath.c_str())) {
                writeException(ex);
            }
        } catch(const webstrada::exception &ex) {
            if (!cfml::cf_cferror_handle(&ex, &m_out, &m_cgi, &m_server, &m_cookie,
                                      &m_application, &m_session, &m_url, &m_form,
                                      &m_variables, requestWebPath.c_str())) {
                // CF returns 404 for a missing request page (EnableHTTPStatus)
                // with the built-in error page; <cferror> never handles it.
                if (ex.m_missingTemplate) {
                    cfml::response().statusCode = 404;
                }
                writeException(ex);
            }
        } catch(const std::runtime_error &ex) {
            FCGX_PutS(ex.what(), request->out);
        } catch(const std::exception &ex) {
            writeException(webstrada::exception("Runtime Exception", ex.what()));
        } catch (...) {
            writeException(webstrada::exception("Unknown error."));
        }

    } catch(const webstrada::abort_exception &ex) {
    } catch(const webstrada::exit_exception &ex) {
    } catch(const webstrada::exception &ex) {
        writeException(ex);
    } catch(const std::exception &ex) {
        writeException(webstrada::exception("Unknown error."));
    }

    // A self-closing <cfcache> miss registered a whole-page store; capture the
    // final response now that the page has completed (CachingFilter).
    cfml::cf_cache_store_page(&m_out);

    // Record the finished request (status + duration) for the dashboard stats.
    // The duration runs from request_begin (start of execution) to here — just
    // before the final output is dumped to the FastCGI stream below — so the
    // time spent encoding/writing the response is excluded.
    int64_t profilerReqId = 0;
    if (webstrada::config::lineExecutionTrace) {
        RequestTraceSummary summary;
        summary.timestamp = static_cast<double>(time(nullptr));
        summary.method = m_cgi.has("REQUEST_METHOD") ? (m_cgi["REQUEST_METHOD"].toString().constData() ? m_cgi["REQUEST_METHOD"].toString().constData() : "GET") : "GET";
        summary.url = m_cgi.has("REQUEST_URI") ? (m_cgi["REQUEST_URI"].toString().constData() ? m_cgi["REQUEST_URI"].toString().constData() : "/") : "/";
        summary.status = cfml::response().statusCode;
        summary.durationMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t_req_start).count();
        summary.onRequestStartMs = g_reqProfiler.onRequestStartTime;
        summary.pageExecutionMs = g_reqProfiler.templateExecTime;
        summary.onRequestEndMs = g_reqProfiler.onRequestEndTime;
        summary.dbQueriesCount = g_reqProfiler.queryCount;
        summary.dbQueriesMs = g_reqProfiler.queryTime;
        summary.customTagsCount = g_reqProfiler.customTagCount;
        summary.customTagsMs = g_reqProfiler.customTagTime;
        summary.cfcMethodsCount = g_reqProfiler.cfcMethodCount;
        summary.cfcMethodsMs = g_reqProfiler.cfcMethodTime;
        summary.steps = cfml::trace_take_steps();

        profilerReqId = m_profilerStore.recordRequest(summary);
    } else {
        cfml::trace_take_steps();
    }

    webstrada::stats::request_end(cfml::response().statusCode, profilerReqId);

    cfml::response_send_remaining(&m_out);
    g_response_stream = nullptr;

    FCGX_Finish_r(request);

    webstrada::db::requestCleanupConnections();

    m_form = cfvariant::Struct;
    m_cgi = cfvariant::Struct;
    m_url = cfvariant::Struct;
    m_cookie = cfvariant::Struct;
    m_variables = cfvariant::Struct;
    m_out.clear();
}

string &worker::out()
{
    return m_out;
}

cfvariant &worker::cgi()
{
    return m_cgi;
}

cfvariant &worker::server()
{
    return m_server;
}

cfvariant &worker::cookie()
{
    return m_cookie;
}

cfvariant &worker::application()
{
    return m_application;
}

cfvariant &worker::session()
{
    return m_session;
}

cfvariant &worker::url()
{
    return m_url;
}

cfvariant &worker::form()
{
    return m_form;
}

cfvariant &worker::variables()
{
    return m_variables;
}

void worker::process_cli_request(const string &pathname, const string &web_root)
{
    cfml::request_set_body(nullptr, 0);

    m_application.setDisabled();
    m_session.setDisabled();

    cfml::scope_begin(&m_scopeStore, &m_application, &m_session);
    struct ScopeSaveGuard {
        ~ScopeSaveGuard() { cfml::scope_end(); }
    } scopeSaveGuard;

    cfml::response_begin();
    cfml::cferror_reset();
    cfml::VariantCleanupGuard guard;

    cfml::IncludeRuntime includeRuntime;
    includeRuntime.webRoot = std::string(web_root.constData());
    includeRuntime.loader = &include_template_loader;
    includeRuntime.loaderOpaque = &m_templates;
    includeRuntime.componentLoader = &component_loader;
    includeRuntime.componentLoaderOpaque = &m_templates;
    cfml::include_begin(&includeRuntime);
    struct IncludeGuard {
        ~IncludeGuard() { cfml::include_end(); }
    } includeGuard;

    const char *docRootEnv = getenv("DOCUMENT_ROOT");
    string app_search_root = docRootEnv ? string(docRootEnv) : string("/");

    // The request's web path (error.template value): the filesystem path
    // with the web root stripped.
    std::string requestWebPath = pathname.constData() ? std::string(pathname.constData(), pathname.length()) : std::string();
    {
        std::string rootStr(web_root.constData() ? web_root.constData() : "");
        while (rootStr.size() > 1 && rootStr.back() == '/') rootStr.pop_back();
        if (!rootStr.empty() && requestWebPath.compare(0, rootStr.size(), rootStr) == 0) {
            requestWebPath = requestWebPath.substr(rootStr.size());
        }
        if (requestWebPath.empty() || requestWebPath[0] != '/') requestWebPath = "/" + requestWebPath;
    }

    // Populate CGI scope for CLI execution.
    m_cgi.setAutoCreate();
    m_cgi["SCRIPT_NAME"] = string(requestWebPath.c_str());
    m_cgi["PATH_INFO"] = "";
    m_cgi["REQUEST_URI"] = string(requestWebPath.c_str());
    m_cgi["DOCUMENT_ROOT"] = web_root;
    m_cgi["REQUEST_METHOD"] = "GET";
    m_cgi["SERVER_PROTOCOL"] = "HTTP/1.1";
    m_cgi.setReadOnly();

    try {
        run_template(pathname, app_search_root);
    } catch (const webstrada::exit_exception &) {
        // <cfexit> at the top level halts the page (output preserved).
    } catch (const webstrada::exception &ex) {
        if (!cfml::cf_cferror_handle(&ex, &m_out, &m_cgi, &m_server, &m_cookie,
                                     &m_application, &m_session, &m_url, &m_form,
                                     &m_variables, requestWebPath.c_str())) {
            throw;
        }
    }

    // Finalize a pending whole-page <cfcache> store (CachingFilter runs after
    // the page completes).
    cfml::cf_cache_store_page(&m_out);

    m_form = cfvariant::Struct;
    m_cgi = cfvariant::Struct;
    m_url = cfvariant::Struct;
    m_cookie = cfvariant::Struct;
    m_variables = cfvariant::Struct;
}


// Throws CF's TemplateNotFoundException equivalent: a top-level missing
// template. In CF this exception extends java.io.FileNotFoundException (not
// MissingIncludeException), so <cferror> handlers never see it (the built-in
// 404 page handles it) — the marker makes cf_cferror_handle skip it.
[[noreturn]] static void throwTemplateNotFound(const webstrada::string &pathname)
{
    webstrada::exception ex("Template not found!", "Error Loading template " + pathname);
    ex.m_missingTemplate = true;
    throw ex;
}

// Executes the requested template. Before it runs, the nearest Application.cfm
// (if any) is executed first, matching ColdFusion: the search starts in the
// template's directory and walks up to the web root; the closest file wins, and
// within one directory Application.cfc takes precedence over Application.cfm
// (a CFC is not executed yet, so the search stops there without running the
// CFM). Both templates share the same output buffer and scopes, so variables
// set by Application.cfm are visible to the page and the whitespace at the
// boundary is managed as between two templates (verified against CF).
void worker::run_template(const string &pathname, const string &web_root)
{
    // A directory containing an Application.cfc shadows Application.cfm; the
    // CFC runs the whole request through its lifecycle methods.
    string app_cfc = find_application_cfc(pathname, web_root);
    cfml::trace_record_event("ENGINE", "[ENGINE]", "FIND_APPLICATION_CFC");
    if (!app_cfc.isEmpty() && !app_cfc.equals(pathname)) {
        if (run_application_cfc(app_cfc, pathname, web_root)) {
            return;
        }
    }

    cfml::IncludeRuntime *inc = cfml::include_context();
    string app_cfm = find_application_cfm(pathname, web_root);
    // If the requested template itself is the Application.cfm, don't run it as
    // the prelude too (ColdFusion refuses direct requests to Application.cfm
    // with an empty server error; running it as the page alone is closest).
    if (!app_cfm.isEmpty() && !app_cfm.equals(pathname)) {
        template_fn app_template = m_templates.get(app_cfm);
        if (app_template != nullptr) {
            if (inc) inc->currentPath = std::string(app_cfm.constData());
            try {
                app_template(&m_out, &m_cgi, &m_server, &m_cookie, &m_application, &m_session, &m_url, &m_form, &m_variables);
            } catch (const webstrada::exit_exception &) {
                // <cfexit> in Application.cfm exits only the prelude page; the
                // requested page still runs (verified on CF).
            }
        }
    }

    // A missing request page is CF's TemplateNotFoundException (which never
    // reaches <cferror>); stat first so the compile path (which would throw a
    // parser error for an unreadable file) is not reached.
    struct stat pageSt;
    template_fn compiled_template = (stat(pathname.constData(), &pageSt) == 0 && S_ISREG(pageSt.st_mode))
        ? m_templates.get(pathname)
        : nullptr;
    if (compiled_template == nullptr) {
        throwTemplateNotFound(pathname);
    }
    if (inc) inc->currentPath = std::string(pathname.constData());
    compiled_template(&m_out, &m_cgi, &m_server, &m_cookie, &m_application, &m_session, &m_url, &m_form, &m_variables);
}

// Locate the nearest Application.cfc for a template (searching from the
// template's directory up to the web root, inclusive, like find_application_cfm
// but for the CFC). Returns "" when none exists.
string worker::find_application_cfc(const string &template_path, const string &web_root)
{
    auto file_exists = [](const string &path) {
        struct stat st;
        return stat(path.constData(), &st) == 0;
    };

    string dir = template_path;
    int slash = dir.lastIndexOf('/');
    if (slash < 0) {
        return "";
    }
    dir = dir.left(slash);

    string root = web_root;
    while (root.length() > 1 && root.endsWith("/")) {
        root.removeLast();
    }

    while (!dir.isEmpty()) {
        string cfc = dir + "/Application.cfc";
        if (file_exists(cfc)) {
            return cfc;
        }
        if (dir.equals(root) || dir.equals("/")) {
            break;
        }
        slash = dir.lastIndexOf('/');
        if (slash < 0) {
            break;
        }
        dir = dir.left(slash);
    }
    return "";
}

// Reads a boolean setting from the Application.cfc's this scope.
static bool appCfcBool(cfvariant *thisScope, const char *key, bool fallback)
{
    if (!thisScope || thisScope->m_type != cfvariant::Struct) return fallback;
    auto it = thisScope->m_struct->find(key);
    if (it == thisScope->m_struct->end()) return fallback;
    return cfml::cfmlBoolean(&it->second, fallback);
}

// Reads a scalar setting from the Application.cfc's this scope (a cfvariant
// pointer to the member, or nullptr when absent).
static cfvariant *appCfcMember(cfvariant *thisScope, const char *key)
{
    if (!thisScope || thisScope->m_type != cfvariant::Struct) return nullptr;
    auto it = thisScope->m_struct->find(key);
    if (it == thisScope->m_struct->end()) return nullptr;
    return &it->second;
}

// Invoke an Application.cfc method by name with the request scopes.
static cfvariant *appCfcInvoke(cfvariant *appCfc, const char *method,
                               const cfvariant **args, int argc,
                               string &out, void *cgi, void *server, void *cookie,
                               void *application, void *session, void *url, void *form)
{
    if (!appCfc || appCfc->m_type != cfvariant::Component || !appCfc->m_component) return nullptr;
    auto t0 = std::chrono::steady_clock::now();
    cfvariant *res = cfml::cf_component_invoke_instance(appCfc->m_component, method, args, argc,
                                              out, cgi, server, cookie, application, session, url, form);
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    fprintf(stderr, "[PROFILE appCfcInvoke] method '%s': %.3f ms\n", method ? method : "(null)", ms);
    fflush(stderr);
    return res;
}

bool worker::run_application_cfc(const string &app_cfc_path, const string &pathname,
                                 const string &web_root)
{
    // Load the Application.cfc definition through the component loader.
    cfml::IncludeRuntime *inc = cfml::include_context();
    if (!inc || !inc->componentLoader) {
        return false;
    }
    webstrada::ComponentInfo *info = inc->componentLoader(app_cfc_path.constData(), inc->componentLoaderOpaque);
    if (!info) return false;
    cfml::trace_record_event("ENGINE", "[ENGINE]", "APPLICATION_CFC_LOAD");

    cfml::VariantCleanupGuard guard;

    // (Re)instantiate the cached application object when the CFC changed.
    if (!m_appCfcPath.equals(app_cfc_path.constData()) || m_appCfc.m_type != cfvariant::Component ||
        !m_appCfc.m_component || m_appCfc.m_component->info != info) {
        cfvariant *inst = cfml::cf_component_instantiate(info, &m_variables,
                                                         &m_out, &m_cgi, &m_server, &m_cookie,
                                                         &m_application, &m_session, &m_url, &m_form);
        m_appCfc = *inst;
        m_appCfcPath = app_cfc_path.constData();
        m_appCfcStarted = false;
        cfml::trace_record_event("ENGINE", "[ENGINE]", "APPLICATION_CFC_INSTANTIATE");
    }
    component_info_release(info);

    if (m_appCfc.m_type != cfvariant::Component || !m_appCfc.m_component) {
        return false;
    }
    cfvariant *thisScope = m_appCfc.m_component->thisScope;
    cfvariant *compPtr = &m_appCfc;

    // Apply the Application.cfc settings like <cfapplication> (name,
    // sessionmanagement, setclientcookies, applicationtimeout, sessiontimeout).
    cfvariant *nameV = appCfcMember(thisScope, "NAME");
    cfvariant *smV = appCfcMember(thisScope, "SESSIONMANAGEMENT");
    cfvariant *sccV = appCfcMember(thisScope, "SETCLIENTCOOKIES");
    cfvariant *atV = appCfcMember(thisScope, "APPLICATIONTIMEOUT");
    cfvariant *stV = appCfcMember(thisScope, "SESSIONTIMEOUT");
    cfvariant *sisV = appCfcMember(thisScope, "SEARCHIMPLICITSCOPES");
    cfvariant *mappingsV = appCfcMember(thisScope, "MAPPINGS");
    if (mappingsV) {
        cfml::app_mappings_set(mappingsV);
    }
    if (smV && cfml::cfmlBoolean(smV, false)) {
        cfml::cf_application_enable(&m_application, &m_session, &m_cookie,
                                    nameV, smV, atV, stV, sccV);
    } else if (nameV) {
        cfml::cf_application_enable(&m_application, &m_session, &m_cookie,
                                    nameV, nullptr, atV, stV, sccV);
    }
    if (sisV) {
        cfml::cf_set_search_implicit_scopes(sisV);
    }
    cfml::trace_record_event("ENGINE", "[ENGINE]", "APPLICATION_ENABLE");

    // onApplicationStart runs once per worker (per app object).
    if (!m_appCfcStarted) {
        m_appCfcStarted = true;
        if (cfml::cf_component_has_method_on(m_appCfc.m_component, "ONAPPLICATIONSTART")) {
            cfvariant *res = appCfcInvoke(compPtr, "onApplicationStart", nullptr, 0,
                                          m_out, &m_cgi, &m_server, &m_cookie,
                                          &m_application, &m_session, &m_url, &m_form);
            if (res && !cfml::cfmlBoolean(res, true)) {
                return true;  // false stops the request
            }
        }
    }

    // onSessionStart runs once per newly-created session.
    if (cfml::scope_context().sessionNewlyCreated &&
        cfml::cf_component_has_method_on(m_appCfc.m_component, "ONSESSIONSTART")) {
        appCfcInvoke(compPtr, "onSessionStart", nullptr, 0,
                     m_out, &m_cgi, &m_server, &m_cookie,
                     &m_application, &m_session, &m_url, &m_form);
    }

    // Check whether the target template exists (for onMissingTemplate). A
    // missing file must not be compiled (TemplateCache::get would fail parsing
    // it); stat first.
    struct stat pageSt;
    bool pageMissing = (stat(pathname.constData(), &pageSt) != 0 || !S_ISREG(pageSt.st_mode));
    template_fn page;
    if (!pageMissing) page = m_templates.get(pathname);
    cfml::trace_record_event("ENGINE", "[ENGINE]", "TARGET_PAGE_RESOLVE");

    // CF passes the request's web path (e.g. "/page.cfm") as targetPage to
    // onRequestStart / onRequest / onMissingTemplate. Derive it from the
    // filesystem path by stripping the web root.
    webstrada::string targetPage = pathname;
    {
        std::string rootStr(web_root.constData() ? web_root.constData() : "");
        while (rootStr.size() > 1 && rootStr.back() == '/') rootStr.pop_back();
        std::string p(pathname.constData() ? pathname.constData() : "");
        if (!rootStr.empty() && p.compare(0, rootStr.size(), rootStr) == 0) {
            p = p.substr(rootStr.size());
        }
        if (p.empty() || p[0] != '/') p = "/" + p;
        targetPage = webstrada::string(p.c_str());
    }

    g_reqProfiler.reset();
    auto reqStart = std::chrono::steady_clock::now();
    double onRequestStartMs = 0;
    double pageMs = 0;
    double onRequestEndMs = 0;

    try {
        // onRequestStart(targetPage): a false return stops the request.
        if (cfml::cf_component_has_method_on(m_appCfc.m_component, "ONREQUESTSTART")) {
            cfml::trace_record_event("ENGINE", "[ENGINE]", "BEFORE_ONREQUESTSTART");
            auto t0 = std::chrono::steady_clock::now();
            cfvariant pageArg(targetPage);
            const cfvariant *args[] = {&pageArg};
            cfvariant *res = appCfcInvoke(compPtr, "onRequestStart", args, 1,
                                          m_out, &m_cgi, &m_server, &m_cookie,
                                          &m_application, &m_session, &m_url, &m_form);
            auto t1 = std::chrono::steady_clock::now();
            onRequestStartMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
            if (res && !cfml::cfmlBoolean(res, true)) {
                return true;  // onRequestStart returned false: stop the request
            }
        }

        bool hasOnRequest = cfml::cf_component_has_method_on(m_appCfc.m_component, "ONREQUEST");
        if (pageMissing && hasOnRequest) {
            // A missing template with onRequest defined routes through
            // onMissingTemplate (CF: onMissingTemplate handles missing pages).
            if (cfml::cf_component_has_method_on(m_appCfc.m_component, "ONMISSINGTEMPLATE")) {
                cfvariant pageArg(targetPage);
                const cfvariant *args[] = {&pageArg};
                appCfcInvoke(compPtr, "onMissingTemplate", args, 1,
                             m_out, &m_cgi, &m_server, &m_cookie,
                             &m_application, &m_session, &m_url, &m_form);
            } else {
                throwTemplateNotFound(pathname);
            }
        } else if (hasOnRequest) {
            // onRequest(targetPage): when defined, the CFC includes the page
            // itself.
            cfvariant pageArg(targetPage);
            const cfvariant *args[] = {&pageArg};
            auto t0 = std::chrono::steady_clock::now();
            appCfcInvoke(compPtr, "onRequest", args, 1,
                         m_out, &m_cgi, &m_server, &m_cookie,
                         &m_application, &m_session, &m_url, &m_form);
            auto t1 = std::chrono::steady_clock::now();
            pageMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        } else if (pageMissing) {
            throwTemplateNotFound(pathname);
        } else {
            if (inc) inc->currentPath = std::string(pathname.constData());
            auto t0 = std::chrono::steady_clock::now();
            page(&m_out, &m_cgi, &m_server, &m_cookie, &m_application, &m_session, &m_url, &m_form, &m_variables);
            auto t1 = std::chrono::steady_clock::now();
            pageMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        // onRequestEnd runs after the page (only on the success path).
        if (cfml::cf_component_has_method_on(m_appCfc.m_component, "ONREQUESTEND")) {
            auto t0 = std::chrono::steady_clock::now();
            appCfcInvoke(compPtr, "onRequestEnd", nullptr, 0,
                         m_out, &m_cgi, &m_server, &m_cookie,
                         &m_application, &m_session, &m_url, &m_form);
            auto t1 = std::chrono::steady_clock::now();
            onRequestEndMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }
    } catch (const webstrada::exception &ex) {
        // onError(exception, eventName) handles an uncaught page exception.
        bool hasOnError = cfml::cf_component_has_method_on(m_appCfc.m_component, "ONERROR");
        if (!hasOnError) throw;
        cfvariant exStruct = cfvariant(cfvariant::Struct);
        exStruct.set("MESSAGE") = ex.m_message;
        exStruct.set("DETAIL") = ex.m_detail;
        exStruct.set("TYPE") = ex.m_type;
        const cfvariant *args[] = {&exStruct, nullptr};
        cfvariant eventName("Request");
        args[1] = &eventName;
        appCfcInvoke(compPtr, "onError", args, 2,
                     m_out, &m_cgi, &m_server, &m_cookie,
                     &m_application, &m_session, &m_url, &m_form);
    }
    auto reqEnd = std::chrono::steady_clock::now();
    double totalReqMs = std::chrono::duration<double, std::milli>(reqEnd - reqStart).count();
    fprintf(stderr,
            "\n[WebStrada Performance Breakdown] Total: %.2f ms\n"
            "  ├─ onRequestStart:        %6.2f ms\n"
            "  ├─ Page Execution:        %6.2f ms\n"
            "  │    ├─ Custom Tags:      %6.2f ms (%d invocations)\n"
            "  │    ├─ CFC Method Calls: %6.2f ms (%d invocations)\n"
            "  │    ├─ CFC Instantiates: %6.2f ms (%d instantiations)\n"
            "  │    └─ DB Queries:       %6.2f ms (%d queries)\n"
            "  └─ onRequestEnd:          %6.2f ms\n",
            totalReqMs,
            onRequestStartMs,
            pageMs,
            g_reqProfiler.customTagTime, g_reqProfiler.customTagCount,
            g_reqProfiler.cfcMethodTime, g_reqProfiler.cfcMethodCount,
            g_reqProfiler.cfcInstantiateTime, g_reqProfiler.cfcInstantiateCount,
            g_reqProfiler.queryTime, g_reqProfiler.queryCount,
            onRequestEndMs);
    fflush(stderr);
    return true;
}

// Locate the Application.cfm that applies to a template, searching from the
// template's directory up to (and including) the web root. The filename is
// case-sensitive ("Application.cfm", exactly as ColdFusion looks it up). In a
// given directory an Application.cfc shadows an Application.cfm (ColdFusion
// runs the CFC); since CFCs are not yet supported, a CFC stops the search and
// no Application.cfm runs. Returns an empty string when no Application.cfm
// should run.
string worker::find_application_cfm(const string &template_path, const string &web_root)
{
    auto file_exists = [](const string &path) {
        struct stat st;
        return stat(path.constData(), &st) == 0;
    };

    // Start from the template's directory.
    string dir = template_path;
    int slash = dir.lastIndexOf('/');
    if (slash < 0) {
        return "";
    }
    dir = dir.left(slash);

    // Normalize the web root (strip trailing slashes) so the comparison below
    // stops the walk at the web root itself, as ColdFusion does.
    string root = web_root;
    while (root.length() > 1 && root.endsWith("/")) {
        root.removeLast();
    }

    while (!dir.isEmpty()) {
        string cfc = dir + "/Application.cfc";
        if (file_exists(cfc)) {
            return "";
        }
        string candidate = dir + "/Application.cfm";
        if (file_exists(candidate)) {
            return candidate;
        }
        if (dir.equals(root) || dir.equals("/")) {
            break;
        }
        slash = dir.lastIndexOf('/');
        if (slash < 0) {
            break;
        }
        dir = dir.left(slash);
    }
    return "";
}

void worker::writeException(const webstrada::exception &ex)
{
    auto to_std_string = [](const webstrada::string &s) -> std::string {
        const char *data = s.constData();
        return data ? std::string(data, s.length()) : std::string();
    };

    string http_user_agent = (m_cgi.has("HTTP_USER_AGENT") ? m_cgi["HTTP_USER_AGENT"].toString() : "").toHtmlEscaped();
    string http_host = (m_cgi.has("HTTP_HOST") ? m_cgi["HTTP_HOST"].toString() : "").toHtmlEscaped();
    string http_referer = (m_cgi.has("HTTP_REFERER") ? m_cgi["HTTP_REFERER"].toString() : "").toHtmlEscaped();

    std::string extra_rows;
    if (m_cgi.has("HTTP_USER_AGENT")) {
        extra_rows += std::format(
            R"html(								<tr>
									<td><font style="COLOR: black; FONT: 8pt/11pt verdana">Browser&nbsp;&nbsp;</td>
									<td><font style="COLOR: black; FONT: 8pt/11pt verdana">{}</td>
								</tr>
)html",
            to_std_string(http_user_agent)
        );
    }
    if (m_cgi.has("HTTP_HOST")) {
        extra_rows += std::format(
            R"html(								<tr>
									<td><font style="COLOR: black; FONT: 8pt/11pt verdana">Remote Address&nbsp;&nbsp;</td>
									<td><font style="COLOR: black; FONT: 8pt/11pt verdana">{}</td>
								</tr>
)html",
            to_std_string(http_host)
        );
    }
    if (m_cgi.has("HTTP_REFERER")) {
        extra_rows += std::format(
            R"html(								<tr>
									<td><font style="COLOR: black; FONT: 8pt/11pt verdana">Referrer&nbsp;&nbsp;</td>
									<td><font style="COLOR: black; FONT: 8pt/11pt verdana">{}</td>
								</tr>
)html",
            to_std_string(http_referer)
        );
    }

    static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    auto now = std::chrono::system_clock::now();
    std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
    auto duration = now.time_since_epoch();
    int millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    std::tm tm_struct;
    std::string datetime_str = "dd-MMM-yy hh:mm:ss.lll AP";
    if (localtime_r(&time_t_now, &tm_struct)) {
        char buf[64];
        const char* month_name = (tm_struct.tm_mon >= 0 && tm_struct.tm_mon < 12) ? months[tm_struct.tm_mon] : "Jan";
        int year_2digit = tm_struct.tm_year % 100;
        int hour12 = tm_struct.tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* meridian = (tm_struct.tm_hour >= 12) ? "PM" : "AM";
        
        std::snprintf(buf, sizeof(buf), "%02d-%s-%02d %02d:%02d:%02d.%03d %s",
                      tm_struct.tm_mday,
                      month_name,
                      year_2digit,
                      hour12,
                      tm_struct.tm_min,
                      tm_struct.tm_sec,
                      millis,
                      meridian);
        datetime_str = buf;
    }

    // The template path + line where the error occurred (the innermost frame of
    // the captured call stack) and the full CFML call stack, mirroring the
    // "The error occurred in <template>: line N." line CF shows under Robust
    // Exception Information.
    auto html_escape = [](const std::string &s) -> std::string {
        std::string r;
        r.reserve(s.size());
        for (char c : s) {
            if (c == '&') r += "&amp;";
            else if (c == '<') r += "&lt;";
            else if (c == '>') r += "&gt;";
            else if (c == '"') r += "&quot;";
            else if (c == '\'') r += "&#39;";
            else r += c;
        }
        return r;
    };

    std::string location_html;
    std::string stack_html;
    if (!ex.m_stackTrace.empty()) {
        const auto &innermost = ex.m_stackTrace.back();
        location_html = " <br><br>The error occurred in ";
        location_html += html_escape(innermost.path);
        location_html += ": line ";
        location_html += std::to_string(innermost.line);
        location_html += ".";

        stack_html =
            "						<tr>\n"
            "							<td colspan=\"2\">\n"
            "								<font style=\"COLOR: black; FONT: 8pt/11pt verdana\">\n"
            "									<b>CFML Call Stack</b>\n"
            "									<br>\n"
            "									<table border=\"0\" cellpadding=\"2\" cellspacing=\"0\">\n"
            "										<tr>\n"
            "											<td><font style=\"COLOR: black; FONT: 8pt/11pt verdana\"><b>Template</b></td>\n"
            "											<td><font style=\"COLOR: black; FONT: 8pt/11pt verdana\"><b>Line</b></td>\n"
            "											<td><font style=\"COLOR: black; FONT: 8pt/11pt verdana\"><b>Function</b></td>\n"
            "										</tr>\n";
        for (auto it = ex.m_stackTrace.rbegin(); it != ex.m_stackTrace.rend(); ++it) {
            stack_html += "										<tr>\n";
            stack_html += "											<td><font style=\"COLOR: black; FONT: 8pt/11pt verdana\">" + html_escape(it->path) + "</td>\n";
            stack_html += "											<td><font style=\"COLOR: black; FONT: 8pt/11pt verdana\">" + std::to_string(it->line) + "</td>\n";
            stack_html += "											<td><font style=\"COLOR: black; FONT: 8pt/11pt verdana\">" + html_escape(it->function) + "</td>\n";
            stack_html += "										</tr>\n";
        }
        stack_html +=
            "									</table>\n"
            "								</font>\n"
            "							</td>\n"
            "						</tr>\n";
    }

    std::string html = std::format(R"html(<font style="COLOR: black; FONT: 16pt/18pt verdana">The web site you are accessing has experienced an unexpected error.<br>Please contact the website administrator.</font>
<br>
<br>
<table border="1" cellpadding="3" bordercolor="#000808" bgcolor="#e7e7e7">
	<tr>
		<td bgcolor="#000066">
			<font style="COLOR: white; FONT: 11pt/13pt verdana" color="white">The following information is meant for the website developer for debugging purposes.</font>
		</td>
	<tr>
	<tr>
		<td bgcolor="#4646EE">
			<font style="COLOR: white; FONT: 11pt/13pt verdana" color="white">Error Occurred While Processing Request</font>
		</td>
	</tr>
	<tr>
		<td>
			<font style="COLOR: black; FONT: 8pt/11pt verdana">
				<table width="500" cellpadding="0" cellspacing="0" border="0">
					<tr>
						<td id="tableProps2" align="left" valign="middle" width="500">
							<h1 id="textSection1" style="COLOR: black; FONT: 13pt/15pt verdana">{}</h1>
							{}
						</td>
					</tr>
					<tr>
						<td id="tablePropsWidth" width="400" colspan="2">
							<font style="COLOR: black; FONT: 8pt/11pt verdana"></font>
						</td>
					</tr>
					<tr>
						<td height>&nbsp;</td>
					</tr>
					<tr>
						<td colspan="2">
							<font style="COLOR: black; FONT: 8pt/11pt verdana">
								Resources:
								<ul>
									<li>Enable Robust Exception Information to provide greater detail about the source of errors.  In the Administrator, click Debugging & Logging > Debug Output Settings, and select the Robust Exception Information option.</li>
									<li>Check the <a href='http://www.bokicsoft.com/webstrada/docs/' target="new">ColdFusion documentation</a> to verify that you are using the correct syntax.</li>
									<li>Search the <a href='http://www.bokicsoft.com/webstrada/kb/' target="new">Knowledge Base</a> to find a solution to your problem.</li>
								</ul>
							</font>
						</td>
					</tr>
					<tr>
						<td colspan="2">
							<table border="0" cellpadding="0" cellspacing="0">
{}								<tr>
									<td><font style="COLOR: black; FONT: 8pt/11pt verdana">Date/Time&nbsp;&nbsp;</td>
									<td><font style="COLOR: black; FONT: 8pt/11pt verdana">{}</td>
								</tr>
							</table>
						</td>
					</tr>
{}
				</table>
			</font>
		</td>
	</tr>
</table>
)html",
        to_std_string(ex.m_message.toHtmlEscaped()),
        to_std_string(ex.m_detail.toHtmlEscaped()) + location_html,
        extra_rows,
        datetime_str,
        stack_html
    );

    m_out.append(html.c_str());
}

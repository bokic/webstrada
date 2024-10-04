#include <webstrada/llvm_codegen.h>
#include <webstrada/template_cache.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/exceptions.h>
#include <webstrada/cf8.h>
#include <webstrada/scope_store.h>
#include <webstrada/cache_store.h>
#include <webstrada/config.h>
#include <webstrada/worker.h>

#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <string>
#include <print>

#include <unistd.h>
#include <sys/stat.h>


int main(int argc, char** argv)
{
    bool use_stdin = false;
    webstrada::string pathname;

    if (argc == 2) {
        std::string arg = argv[1];
        if (arg == "--stdin" || arg == "-s") {
            use_stdin = true;
        } else {
            pathname = argv[1];
        }
    } else {
        std::println(stderr, "Usage: {} [--stdin | filename]", argv[0]);
        return EXIT_FAILURE;
    }

    // The CLI's stdout is the page output (compared byte-for-byte against
    // ColdFusion by verify_with_coldfusion.py), so <cfquery> logs must not
    // leak into it.
    webstrada::config::enableQueryLogging = false;
    webstrada::config::loadDatasourcesFromEnv();
    cfml::seed_rand();

    // Buffered page output accumulated so far; flushed on an uncaught
    // exception too (CF emits the partial page, e.g. a finally block's output,
    // before the error — previously the CLI discarded it, was BUGS.md
    // "WebStrada-cli discards buffered output").
    std::vector<char> pendingOutput;
    bool havePendingOutput = false;

    try {
        bool print_ast = getenv("PRINT_AST") != nullptr;

        if (use_stdin) {
            std::string input_code;
            std::string line;
            while (std::getline(std::cin, line)) {
                input_code += line;
                input_code += '\n';
            }

            if (print_ast) {
                webstrada::llvm_codegen compiler;
                auto compiled = compiler.compile_string(input_code.data(), input_code.size(), true, "stdin");
                if (compiled == nullptr) {
                    std::println(stderr, "Failed to compile template");
                    return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
            }

            webstrada::TemplateCache templates;
            webstrada::string output;

            webstrada::cfvariant cgi = webstrada::cfvariant::Struct;
            webstrada::cfvariant server = webstrada::cfvariant::Struct;
            webstrada::cfvariant cookie = webstrada::cfvariant::Struct;
            webstrada::cfvariant application = webstrada::cfvariant::Struct;
            webstrada::cfvariant session = webstrada::cfvariant::Struct;
            webstrada::cfvariant url = webstrada::cfvariant::Struct;
            webstrada::cfvariant form = webstrada::cfvariant::Struct;
            webstrada::cfvariant variables = webstrada::cfvariant::Struct;

            cfml::VariantCleanupGuard guard;
            cfml::response_begin();

            webstrada::ScopeStore scopeStore;
            {
                char dbPath[] = "/tmp/WebStrada-cli-scopes-XXXXXX";
                int fd = mkstemp(dbPath);
                if (fd != -1) close(fd);
                scopeStore.open(dbPath);
                unlink(dbPath);
            }
            cfml::scope_begin(&scopeStore, &application, &session);

            // Cache store: the cache functions need a writable store too. Use a
            // temp file so CLI runs never touch the daemon's real cache DB.
            {
                char dbPath[] = "/tmp/WebStrada-cli-cache-XXXXXX";
                int fd = mkstemp(dbPath);
                if (fd != -1) close(fd);
                unlink(dbPath);
                webstrada::config::cacheDbPath = dbPath;
                webstrada::open_cache_store();
            }

            cfml::IncludeRuntime includeRuntime;
            includeRuntime.currentPath = (std::filesystem::current_path() / "stdin.cfm").string();
            includeRuntime.webRoot = std::filesystem::current_path().string();
            includeRuntime.loader = [](const char *path, void *opaque) -> cfml::include_template_fn {
                webstrada::TemplateCache *cache = static_cast<webstrada::TemplateCache*>(opaque);
                if (!cache) return nullptr;
                struct stat st;
                if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return nullptr;
                webstrada::template_fn fn = cache->get(webstrada::string(path));
                auto *target = fn.target<cfml::include_template_fn>();
                return target ? *target : nullptr;
            };
            includeRuntime.loaderOpaque = &templates;
            includeRuntime.componentLoader = [](const char *path, void *opaque) -> webstrada::ComponentInfo* {
                webstrada::TemplateCache *cache = static_cast<webstrada::TemplateCache*>(opaque);
                if (!cache) return nullptr;
                struct stat st;
                if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return nullptr;
                return cache->get_component(webstrada::string(path));
            };
            includeRuntime.componentLoaderOpaque = &templates;
            cfml::include_begin(&includeRuntime);

            webstrada::llvm_codegen compiler;
            auto compiled = compiler.compile_string(input_code.data(), input_code.size(), false, "stdin");
            if (compiled == nullptr) {
                std::println(stderr, "Failed to compile template");
                return EXIT_FAILURE;
            }

            try {
                compiled(&output, &cgi, &server, &cookie, &application,
                         &session, &url, &form, &variables);
            } catch (const webstrada::abort_exception &ex) {
                // cfabort — flush output that was written before the abort
            } catch (const webstrada::exit_exception &ex) {
                // <cfexit> at the top level — flush output written before it
            } catch (...) {
                // Capture the partial output so the outer handler can flush it
                // (CF emits the partial page before the error).
                pendingOutput = cfml::response_encode_all(output);
                havePendingOutput = true;
                throw;
            }

            // Finalize a pending whole-page <cfcache> store (CachingFilter).
            cfml::cf_cache_store_page(&output);

            cfml::include_end();
            cfml::scope_end();

            pendingOutput = cfml::response_encode_all(output);
            havePendingOutput = true;
            std::cout.write(pendingOutput.data(), pendingOutput.size());
            return EXIT_SUCCESS;
        }

        if (print_ast) {
            webstrada::llvm_codegen compiler;
            auto compiled = compiler.compile(pathname, true);
            if (compiled == nullptr) {
                std::println(stderr, "Failed to compile template");
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }

        std::filesystem::path tplPath = std::filesystem::absolute(pathname.constData());
        const char *docRootEnv = getenv("DOCUMENT_ROOT");
        std::string webRootStr = docRootEnv ? docRootEnv : tplPath.parent_path().string();

        webstrada::worker w;
        try {
            w.process_cli_request(tplPath.string().c_str(), webRootStr.c_str());
        } catch (const webstrada::abort_exception &ex) {
            // cfabort / cflocation — flush output that was written before the
            // abort, then exit 0 (the stdin path does the same below).
        } catch (const webstrada::exit_exception &ex) {
            // <cfexit> at the top level — flush output that was written before
            // it and exit 0 (the stdin path does the same below).
        } catch (...) {
            // Capture the partial output so the outer handler can flush it.
            pendingOutput = cfml::response_encode_all(w.out());
            havePendingOutput = true;
            throw;
        }

        std::vector<char> bytes = cfml::response_encode_all(w.out());
        pendingOutput = bytes;
        havePendingOutput = true;
        std::cout.write(bytes.data(), bytes.size());
    } catch (const webstrada::exception &ex) {
        // constData() is nullptr for an empty string; std::format calls
        // strlen() on the pointer, so pass a non-null literal for the empty
        // message case.
        if (havePendingOutput) {
            std::cout.write(pendingOutput.data(), pendingOutput.size());
        }
        std::println(stderr, "Error: {}", ex.m_message.isEmpty() ? "" : ex.m_message.constData());
        if (!ex.m_detail.isEmpty()) {
            std::println(stderr, "Detail: {}", ex.m_detail.constData());
        }
        return EXIT_FAILURE;
    } catch (const std::exception &ex) {
        if (havePendingOutput) {
            std::cout.write(pendingOutput.data(), pendingOutput.size());
        }
        std::println(stderr, "Error: {}", ex.what());
        return EXIT_FAILURE;
    } catch (...) {
        if (havePendingOutput) {
            std::cout.write(pendingOutput.data(), pendingOutput.size());
        }
        std::println(stderr, "Error: Unknown exception occurred.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

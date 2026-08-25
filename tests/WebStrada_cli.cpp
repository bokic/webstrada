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
#include "../src/cftags/common.h"

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

            char tmpPath[] = "/tmp/WebStrada-stdin-XXXXXX.cfm";
            int fd = mkstemps(tmpPath, 4);
            if (fd == -1) {
                std::println(stderr, "Failed to create temp file for stdin");
                return EXIT_FAILURE;
            }
            write(fd, input_code.data(), input_code.size());
            close(fd);

            std::filesystem::path tplPath = std::filesystem::absolute(tmpPath);
            const char *docRootEnv = getenv("DOCUMENT_ROOT");
            std::string webRootStr = docRootEnv ? docRootEnv : std::filesystem::current_path().string();

            webstrada::worker w;
            try {
                w.process_cli_request(tplPath.string().c_str(), webRootStr.c_str());
            } catch (const webstrada::abort_exception &ex) {
            } catch (const webstrada::exit_exception &ex) {
            } catch (...) {
                unlink(tmpPath);
                pendingOutput = cfml::response_encode_all(w.out());
                havePendingOutput = true;
                throw;
            }
            unlink(tmpPath);

            pendingOutput = cfml::response_encode_all(w.out());
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

/**
 * @file tag_execute.cpp
 * @brief <cfexecute> runtime (cf_execute_tag).
 *
 * Runs an external process and captures its stdout/stderr to the page, a
 * variable, or a file. Mirrors CF 2025's ExecuteTag + ProcessExecutor: string
 * arguments are tokenized with the Java getProcessCommand rules (space
 * separated, double-quoted segments kept as one argument), the timeout limits
 * how long the tag waits for the process output, and a timeout of 0 (absent)
 * means the tag does NOT wait for the process at all (ProcessExecutor's
 * no-blocking race — verified on CF 2025 the output is then not captured).
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>

namespace {

using webstrada::cfvariant;
using webstrada::string;

// Read an attribute from the evaluated attribute struct (case-insensitive).
const cfvariant *attrOf(const cfvariant *attrs, const char *key)
{
    if (!attrs || attrs->m_type != cfvariant::Struct || !attrs->m_struct) return nullptr;
    string k(key);
    auto it = attrs->m_struct->find(k);
    return it == attrs->m_struct->end() ? nullptr : &it->second;
}

std::string attrStr(const cfvariant *attrs, const char *key)
{
    const cfvariant *v = attrOf(attrs, key);
    return v ? cfml::safe_to_std_string(*v) : std::string();
}

[[noreturn]] void throwApp(const std::string &message, const std::string &detail = "")
{
    throw webstrada::exception(webstrada::string("Application"),
        webstrada::string(message.c_str()), webstrada::string(detail.c_str()));
}

// Resolve a relative output/error file path against the CFML temporary
// directory (CF's setOutputfile/setErrorfile use Utils.getFileFullPath, which
// for a relative path resolves against the temp directory).
std::string resolveOutputPath(const std::string &path)
{
    if (path.empty()) return path;
    if (path[0] == '/' || (path.size() > 1 && path[1] == ':')) return path;
    std::filesystem::path tmp = std::filesystem::temp_directory_path();
    return (tmp / path).string();
}

// Tokenize a cfexecute string `arguments` value on Unix like CF's
// ExecuteTag.getProcessCommand: the default separator is a space, double-quoted
// segments with embedded spaces are kept as one argument, and a quote in the
// middle of an unquoted argument is literal.
void tokenizeUnixArguments(const std::string &args, std::vector<std::string> &out)
{
    std::string argument;
    bool quoting = false;
    for (size_t i = 0; i < args.size(); i++) {
        char c = args[i];
        if (c == '"' && (argument.empty() || quoting)) {
            if (quoting) {
                out.push_back(argument);
                argument.clear();
            }
            quoting = !quoting;
        } else if (c == ' ' && !quoting) {
            if (!argument.empty()) {
                out.push_back(argument);
                argument.clear();
            }
        } else {
            argument += c;
        }
    }
    if (!argument.empty()) out.push_back(argument);
}

// Where a reader thread writes the data: into a memory buffer or directly into
// a file. Shared via shared_ptr so the detached reader threads stay alive after
// cf_execute_tag unwinds (the timeout path throws while the threads still run).
struct StreamSink {
    std::string data;
    std::string filePath;

    std::mutex mtx;
    bool done = false;
};

// Reader thread body: drains fd to EOF, appending to the sink. When the sink
// has a file path, the bytes are written there directly. Once the pipe hits EOF
// the child process has exited, so one of the two reader threads reaps it.
void readerThreadBody(int fd, std::shared_ptr<StreamSink> sink, pid_t pid)
{
    char tmp[4096];
    std::ofstream file;
    if (!sink->filePath.empty()) {
        file.open(sink->filePath, std::ios::binary | std::ios::trunc);
    }
    for (;;) {
        ssize_t n = ::read(fd, tmp, sizeof(tmp));
        if (n <= 0) break;
        if (file.is_open()) {
            file.write(tmp, n);
        } else {
            std::lock_guard<std::mutex> lock(sink->mtx);
            sink->data.append(tmp, static_cast<size_t>(n));
        }
    }
    if (file.is_open()) file.close();
    ::close(fd);
    if (pid > 0) {
        int wstatus = 0;
        ::waitpid(pid, &wstatus, 0);   // ECHILD is fine for the second thread
    }
    std::lock_guard<std::mutex> lock(sink->mtx);
    sink->done = true;
}

} // namespace

namespace cfml {

void cf_execute_tag(string *out, const cfvariant *attrs,
                    void *cgi, void *server, void *cookie, void *application,
                    void *session, void *url, void *form, void *variables)
{
    (void)cgi; (void)server; (void)cookie; (void)application; (void)session;
    (void)url; (void)form; (void)variables;

    std::string name = attrStr(attrs, "name");
    const cfvariant *argsVal = attrOf(attrs, "arguments");

    // Build the command: the executable + the tokenized arguments.
    std::vector<std::string> cmd;
    cmd.push_back(name);
    if (argsVal) {
        if (argsVal->m_type == cfvariant::Array && argsVal->m_array) {
            for (const auto &el : *argsVal->m_array) {
                cmd.push_back(safe_to_std_string(el));
            }
        } else {
            tokenizeUnixArguments(safe_to_std_string(*argsVal), cmd);
        }
    }

    std::string outFile = resolveOutputPath(attrStr(attrs, "outputfile"));
    std::string errFile = resolveOutputPath(attrStr(attrs, "errorfile"));
    std::string varName = attrStr(attrs, "variable");
    std::string errVarName = attrStr(attrs, "errorvariable");

    long timeoutMs = 0;
    if (const cfvariant *t = attrOf(attrs, "timeout")) {
        std::string ts = safe_to_std_string(*t);
        // CF's ExecuteTag.setTimeout does `timeout = s * 1000` with s an int
        // (Cast._int): a leading numeric value is truncated, a non-numeric
        // value throws CF's Expression "The value X cannot be converted to a
        // number." (verified on CF 2025 for a dynamic timeout value).
        std::string trimmed = ts;
        size_t b = trimmed.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) trimmed.clear();
        else trimmed = trimmed.substr(b);
        if (!trimmed.empty() &&
            (std::isdigit(static_cast<unsigned char>(trimmed[0])) ||
             trimmed[0] == '-' || trimmed[0] == '+' || trimmed[0] == '.')) {
            try {
                size_t idx = 0;
                double d = std::stod(trimmed, &idx);
                // Cast._int truncates toward zero.
                timeoutMs = static_cast<long>(d) * 1000;
            } catch (...) {
                throw webstrada::exception(webstrada::string(("The value " + ts +
                    " cannot be converted to a number.").c_str()));
            }
        } else {
            throw webstrada::exception(webstrada::string(("The value " + ts +
                " cannot be converted to a number.").c_str()));
        }
    }

    // Build argv: execvp needs a NULL-terminated char* array.
    std::vector<char*> argv;
    for (auto &c : cmd) argv.push_back(const_cast<char*>(c.c_str()));
    argv.push_back(nullptr);

    // Create the stdout/stderr pipes.
    int outPipe[2], errPipe[2];
    if (::pipe(outPipe) != 0 || ::pipe(errPipe) != 0) {
        throwApp("An exception occurred when invoking an external process.",
                 "The cause of this exception was that: java.io.IOException: Could not create pipes.");
    }

    // Create a status pipe so an exec failure in the child is reported back.
    int statusPipe[2];
    if (::pipe(statusPipe) != 0) {
        throwApp("An exception occurred when invoking an external process.",
                 "The cause of this exception was that: java.io.IOException: Could not create pipes.");
    }

    pid_t pid = ::fork();
    if (pid < 0) {
        throwApp("An exception occurred when invoking an external process.",
                 "The cause of this exception was that: java.io.IOException: " +
                 std::string(::strerror(errno)) + ".");
    }

    if (pid == 0) {
        // Child: wire the pipes and exec.
        ::close(outPipe[0]);
        ::close(errPipe[0]);
        ::close(statusPipe[0]);
        // Mark the status pipe write end close-on-exec so an exec success
        // closes it (EOF to the parent); an exec failure still reports via it.
        fcntl(statusPipe[1], F_SETFD, FD_CLOEXEC);
        if (::dup2(outPipe[1], STDOUT_FILENO) < 0 || ::dup2(errPipe[1], STDERR_FILENO) < 0) {
            const char *em = ::strerror(errno);
            ::write(statusPipe[1], em, ::strlen(em) + 1);
            ::_exit(127);
        }
        ::close(outPipe[1]);
        ::close(errPipe[1]);
        // exec success closes the status fd, so an EOF on the status pipe
        // means exec succeeded.
        int savedErrno = 0;
        ::execvp(argv[0], argv.data());
        savedErrno = errno;
        // Report the failure in Java's format: error=N, <message>.
        std::string msg = "error=" + std::to_string(savedErrno) + ", " + ::strerror(savedErrno);
        ::write(statusPipe[1], msg.c_str(), msg.size() + 1);
        ::_exit(127);
    }

    // Parent.
    ::close(outPipe[1]);
    ::close(errPipe[1]);
    ::close(statusPipe[1]);

    // Did the exec fail? Read the status pipe; EOF (0 bytes) means exec
    // succeeded (the child exec'd and the status fd was closed by exec).
    char statusBuf[512];
    ssize_t st = ::read(statusPipe[0], statusBuf, sizeof(statusBuf) - 1);
    ::close(statusPipe[0]);
    if (st > 0) {
        // exec failed.
        statusBuf[st] = 0;
        std::string errStr = statusBuf;
        // Reap the child (it already exited).
        int wstatus = 0;
        ::waitpid(pid, &wstatus, 0);
        // CF wraps the java.io.IOException message ("Cannot run program
        // \"<name>\": error=N, <strerror>.").
        std::string detail = "The cause of this exception was that: java.io.IOException: "
                             "Cannot run program \"" + name + "\": " + errStr + ".";
        throwApp("An exception occurred when invoking an external process.", detail);
    }

    // Read stdout/stderr concurrently via reader threads (CF's ProcessReaderThread).
    // The sinks are shared_ptr so a detached thread that outlives this function
    // (timeout/no-timeout paths) keeps them alive safely.
    auto outSink = std::make_shared<StreamSink>();
    auto errSink = std::make_shared<StreamSink>();
    if (outFile.empty() && !varName.empty()) {
        // keep the accumulated output in the sink's buffer
    } else if (!outFile.empty()) {
        outSink->filePath = outFile;
    }
    if (!errFile.empty()) {
        errSink->filePath = errFile;
    }

    std::thread outThread(readerThreadBody, outPipe[0], outSink, pid);
    std::thread errThread(readerThreadBody, errPipe[0], errSink, pid);

    bool timedOut = false;
    bool outputAvailable = false;   // result captured (process completed)
    if (timeoutMs > 0) {
        // Wait up to timeout for BOTH reader threads to finish (the pipes hit
        // EOF, which requires the process to exit). If the deadline passes,
        // CF throws the ProcessTimedOutException.
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        for (;;) {
            bool bothDone = false;
            {
                std::lock_guard<std::mutex> lo(outSink->mtx);
                std::lock_guard<std::mutex> le(errSink->mtx);
                bothDone = outSink->done && errSink->done;
            }
            if (bothDone) break;
            if (std::chrono::steady_clock::now() >= deadline) { timedOut = true; break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        // Detach the reader threads so the child keeps draining its pipes
        // (CF leaves the reader threads running after a timeout).
        outThread.detach();
        errThread.detach();
        if (timedOut) {
            throwApp("The requested action did not complete in a timely manner. Timeout period expired without completion of " + name + ".",
                     "Timeout period expired without completion of " + name + ".");
        }
        outputAvailable = true;
    } else {
        // No timeout: CF does NOT wait for the process (blocking is false and
        // timeout is 0, so neither the waitFor nor the join runs) and reads the
        // result immediately — the reader threads have usually not produced
        // anything yet, so the output is not captured (verified on CF 2025: the
        // variable is not set, and nothing is written to the page). The
        // background threads keep draining the pipes (and writing an
        // outputfile/errorfile), exactly like CF's ProcessReaderThread.
        outThread.detach();
        errThread.detach();
    }

    // Assign the results. Only when the process completed (a positive timeout
    // was reached and both streams hit EOF) is the output captured; the
    // no-timeout race leaves the result null in CF, so no variable is bound.
    if (!outputAvailable) {
        return;
    }
    std::string stdoutData, stderrData;
    if (outFile.empty()) {
        std::lock_guard<std::mutex> lo(outSink->mtx);
        stdoutData = outSink->data;
    }
    if (errFile.empty() && !errVarName.empty()) {
        std::lock_guard<std::mutex> le(errSink->mtx);
        stderrData = errSink->data;
    }
    if (!varName.empty()) {
        cfvariant res(stdoutData.c_str());
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         varName.c_str(), &res);
    } else if (outFile.empty() && out) {
        // No variable and no outputfile: the stdout goes to the page.
        cfwriteoutput(*out, stdoutData.c_str(), stdoutData.size());
    }
    if (!errVarName.empty()) {
        cfvariant res(stderrData.c_str());
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         errVarName.c_str(), &res);
    }
}

} // namespace cfml

/**
 * @file tag_file.cpp
 * @brief <cffile> runtime (cf_file_tag).
 *
 * Implements the read/readBinary/write/append/copy/move/rename/delete/upload
 * actions of <cffile> with Adobe ColdFusion semantics (ported from the
 * decompiled FileTag / FileUtils). The write/append body is captured by the
 * codegen into `bodyContent` (null when the tag has no body / the action is not
 * write/append). After every action the `cffile` struct is created (empty for
 * the non-upload actions; populated for upload), matching CF's file scope.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/upload.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using webstrada::UploadedFile;

namespace cfml {

namespace {

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
    return v ? safe_to_std_string(*v) : std::string();
}

bool attrBool(const cfvariant *attrs, const char *key, bool def)
{
    const cfvariant *v = attrOf(attrs, key);
    return v ? cfmlBoolean(v, def) : def;
}

[[noreturn]] void throwApp(const std::string &message, const std::string &detail = "")
{
    throw webstrada::exception(webstrada::string("Application"),
        webstrada::string(message.c_str()), webstrada::string(detail.c_str()));
}

// The FileUtils.SingleFileOperationException message: the FileNotFoundException
// payload of the wrapped java.io.IOException.
void throwSingleFileOperation(const std::string &action, const std::string &file,
                              const std::string &cause)
{
    throwApp("An error occurred when performing a file operation " + action +
             " on file " + file + ".",
             "The cause of this exception was: " + cause + ".");
}

void throwMultipleFileOperation(const std::string &action, const std::string &source,
                                const std::string &dest, const std::string &cause)
{
    throwApp("An exception occurred when performing a file operation " + action +
             " on files " + source + " and " + dest + ".",
             "The cause of this exception was: " + cause + ".");
}

// Creates the parent directories for a destination (CF's VFS getOutputStream
// creates the full path; write to a path whose parent is missing throws a
// FileNotFoundException).
bool ensureParents(const std::string &path)
{
    std::filesystem::path p(path);
    std::filesystem::path parent = p.parent_path();
    if (parent.empty()) return true;
    std::error_code ec;
    if (!std::filesystem::exists(parent, ec)) {
        std::filesystem::create_directories(parent, ec);
        if (ec) return false;
    }
    return true;
}

// CF's createNewFile content writer honoring fixnewline / addnewline.
void writeContentToFile(const std::string &file, const std::string &content,
                        bool addNewLine, bool fixnewline, const std::string &charset)
{
    (void)charset;   // the engine has no charset conversion pipeline yet
    if (fixnewline) {
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        std::string line;
        size_t i = 0;
        bool first = true;
        while (i <= content.size()) {
            size_t nl = content.find('\n', i);
            if (nl == std::string::npos) {
                line = content.substr(i);
                i = content.size() + 1;
            } else {
                line = content.substr(i, nl - i);
                i = nl + 1;
            }
            if (!first) out << "\n";
            first = false;
            out << line;
            if (i > content.size()) break;
        }
        if (addNewLine) out << "\n";
        out.close();
        return;
    }
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << content;
    if (addNewLine) out << "\n";
    out.close();
}

void appendContentToFile(const std::string &file, const std::string &content,
                         bool addNewLine, const std::string &charset)
{
    (void)charset;
    std::ofstream out(file, std::ios::binary | std::ios::app);
    out << content;
    if (addNewLine) out << "\n";
    out.close();
}

std::string readFileContents(const std::string &file, const std::string &charset)
{
    (void)charset;
    std::ifstream in(file, std::ios::binary);
    if (!in) return std::string();
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// CF's FileUtils.determineDest: if `destStr` is an existing directory the file
// lands inside it with `sourceName`; otherwise (when it doesn't end with a
// slash) it is treated as the full target path.
std::string determineDest(const std::string &destStr, const std::string &sourceName)
{
    std::error_code ec;
    if (std::filesystem::is_directory(destStr, ec)) {
        return (std::filesystem::path(destStr) / sourceName).string();
    }
    if (!destStr.empty() && (destStr.back() == '/' || destStr.back() == '\\')) {
        return (std::filesystem::path(destStr) / sourceName).string();
    }
    return destStr;
}

// Set Unix file permissions (CF's setUnixModes via chmod). `mode` is an octal
// string like "644"; -1 / empty means no change.
void setUnixModes(const std::string &path, const std::string &mode)
{
    if (mode.empty()) return;
    std::string m = mode;
    m = m.substr(m.find_first_not_of(" \t\r\n"));
    if (m.empty()) return;
    long octal = 0;
    bool ok = !m.empty();
    for (char c : m) {
        if (c < '0' || c > '7') { ok = false; break; }
        octal = octal * 8 + (c - '0');
    }
    if (!ok) return;
    ::chmod(path.c_str(), static_cast<mode_t>(octal));
}

// A Boolean cfvariant. NOTE: `cfvariant(bool)` would resolve to the
// cfvariantType enum constructor (NotSet/Null), so a real Boolean must be
// built explicitly.
cfvariant boolVar(bool b)
{
    cfvariant v(cfvariant::Boolean);
    v.m_bool = b;
    return v;
}

// Assign the empty `cffile` result struct (created after every action).
void createEmptyCffileStruct(void *cgi, void *server, void *cookie, void *application,
                             void *session, void *url, void *form, void *variables)
{
    cfvariant res(cfvariant::Struct);
    cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                     static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                     static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                     static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                     "cffile", &res);
}

} // namespace

// <cffile> runtime entry. `attrs` holds the evaluated attributes with lowercased
// keys; `bodyContent` is the captured tag body for write/append (may be null).
void cf_file_tag(const cfvariant *attrs, void *cgi, void *server, void *cookie,
                 void *application, void *session, void *url, void *form,
                 void *variables, webstrada::string *bodyContent)
{
    // The captured tag body lives on the silent-buffer stack (pushed by the
    // codegen's cf_silent_begin). Copy the content and pop the buffer (the same
    // contract as cf_xml_end) so the pointer stays valid until we read it.
    std::string bodyStr;
    if (bodyContent) {
        const char *d = bodyContent->constData();
        if (d) bodyStr.assign(d, bodyContent->length());
        silent_buf_pop();
    }

    std::string action = attrStr(attrs, "action");
    std::string lowerAction;
    for (char c : action) lowerAction += static_cast<char>(tolower((unsigned char)c));

    std::string file = attrStr(attrs, "file");
    std::string source = attrStr(attrs, "source");
    std::string destination = attrStr(attrs, "destination");
    std::string variable = attrStr(attrs, "variable");
    std::string charset = attrStr(attrs, "charset");
    std::string attributes = attrStr(attrs, "attributes");
    std::string mode = attrStr(attrs, "mode");
    std::string nameconflict = attrStr(attrs, "nameconflict");
    std::string result = attrStr(attrs, "result");
    bool addnewline = attrBool(attrs, "addnewline", true);
    bool fixnewline = attrBool(attrs, "fixnewline", false);

    if (lowerAction == "read") {
        if (!std::filesystem::exists(file)) {
            throwSingleFileOperation("read", file,
                "java.io.FileNotFoundException: " + file + " (No such file or directory).");
        }
        std::string content = readFileContents(file, charset);
        cfvariant val(content.c_str());
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         variable.c_str(), &val);
        createEmptyCffileStruct(cgi, server, cookie, application, session, url, form, variables);
        return;
    }

    if (lowerAction == "readbinary") {
        if (!std::filesystem::exists(file)) {
            throwSingleFileOperation("readBinary", file,
                "java.io.FileNotFoundException: " + file + " (No such file or directory).");
        }
        std::ifstream in(file, std::ios::binary);
        std::vector<char> raw((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
        std::vector<std::byte> bytes;
        bytes.reserve(raw.size());
        for (char c : raw) bytes.push_back(static_cast<std::byte>(c));
        cfvariant val(cfvariant::Binary);
        *val.m_binary = std::move(bytes);
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         variable.c_str(), &val);
        createEmptyCffileStruct(cgi, server, cookie, application, session, url, form, variables);
        return;
    }

    if (lowerAction == "write" || lowerAction == "append") {
        std::string content;
        const cfvariant *output = attrOf(attrs, "output");
        bool hasBody = !bodyStr.empty();
        std::string outputStr;
        if (output) outputStr = safe_to_std_string(*output);

        // If the body is non-blank and the output attribute is present, CF
        // rejects the combination.
        if (hasBody && output && !outputStr.empty()) {
            throwApp("Output attribute cannot be used when content is specified inside file tag body.");
        }
        if (hasBody) content = bodyStr;
        else if (output) content = outputStr;
        else {
            std::string actUp;
            for (char c : lowerAction) actUp += static_cast<char>(toupper((unsigned char)c));
            throwApp("Attribute validation error for CFFILE.",
                     "When the value of the ACTION attribute is " + actUp +
                     ", it requires a Tag Body or the attribute(s): OUTPUT.");
        }

        try {
            if (lowerAction == "write") {
                if (!ensureParents(file)) {
                    throwSingleFileOperation("write", file,
                        "java.io.FileNotFoundException: " + file + " (No such file or directory).");
                }
                writeContentToFile(file, content, addnewline, fixnewline, charset);
            } else {
                if (!std::filesystem::exists(file)) {
                    if (!ensureParents(file)) {
                        throwSingleFileOperation("append", file,
                            "java.io.FileNotFoundException: " + file + " (No such file or directory).");
                    }
                    writeContentToFile(file, content, addnewline, fixnewline, charset);
                } else {
                    appendContentToFile(file, content, addnewline, charset);
                }
            }
            setUnixModes(file, mode);
        } catch (const std::exception &e) {
            const char *w = e.what();
            throwSingleFileOperation(lowerAction, file, w ? w : "");
        }
        createEmptyCffileStruct(cgi, server, cookie, application, session, url, form, variables);
        return;
    }

    if (lowerAction == "copy") {
        std::string s = source.empty() ? file : source;
        std::string d = destination.empty() ? file : destination;
        std::string sLow, dLow;
        for (char c : s) sLow += static_cast<char>(tolower((unsigned char)c));
        for (char c : d) dLow += static_cast<char>(tolower((unsigned char)c));
        if (sLow == dLow) {
            throwApp("You cannot copy the file " + s + " over itself.");
        }
        if (!std::filesystem::exists(s)) {
            throwMultipleFileOperation("copy", s, d,
                "java.io.FileNotFoundException: " + s + " (No such file or directory).");
        }
        std::string target = determineDest(d, std::filesystem::path(s).filename().string());
        std::error_code ec;
        if (!ensureParents(target)) {
            throwMultipleFileOperation("copy", s, d,
                "java.io.FileNotFoundException: " + target + " (No such file or directory).");
        }
        std::filesystem::copy_file(s, target, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            throwMultipleFileOperation("copy", s, d,
                "java.io.FileNotFoundException: " + target + " (No such file or directory).");
        }
        setUnixModes(target, mode);
        createEmptyCffileStruct(cgi, server, cookie, application, session, url, form, variables);
        return;
    }

    if (lowerAction == "move") {
        std::string s = source.empty() ? file : source;
        std::string d = destination.empty() ? file : destination;
        if (!std::filesystem::exists(s)) {
            throwApp("The source " + s + " specified in the cffile tag is invalid.");
        }
        std::string target = determineDest(d, std::filesystem::path(s).filename().string());
        std::error_code ec;
        if (!ensureParents(target)) {
            throwApp("The source " + s + " specified in the cffile tag is invalid.");
        }
        std::filesystem::rename(s, target, ec);
        if (ec) {
            throwApp("The source " + s + " specified in the cffile tag is invalid.");
        }
        setUnixModes(target, mode);
        createEmptyCffileStruct(cgi, server, cookie, application, session, url, form, variables);
        return;
    }

    if (lowerAction == "rename") {
        std::string s = source.empty() ? file : source;
        std::string d = destination.empty() ? file : destination;
        if (!std::filesystem::exists(s)) {
            throwApp("Attribute validation error for tag CFFILE.",
                     "The value of the attribute source, which is currently " + s + ", is invalid.");
        }
        std::string target = determineDest(d, std::filesystem::path(s).filename().string());
        std::error_code ec;
        std::filesystem::rename(s, target, ec);
        if (ec) {
            throwApp("Attribute validation error for tag CFFILE.",
                     "The value of the attribute source, which is currently " + s + ", is invalid.");
        }
        setUnixModes(target, mode);
        createEmptyCffileStruct(cgi, server, cookie, application, session, url, form, variables);
        return;
    }

    if (lowerAction == "delete") {
        if (std::filesystem::exists(file)) {
            std::filesystem::remove(file);
        } else {
            // CF throws FileDoesNotExistException only when the file is not
            // writable AND does not exist; on Linux a missing file is not
            // writable, so the error fires.
            throwApp("File " + file + " specified in action delete does not exist.");
        }
        createEmptyCffileStruct(cgi, server, cookie, application, session, url, form, variables);
        return;
    }

    if (lowerAction == "upload") {
        // The upload path reads the request-scoped multipart files
        // (UploadRegistry). Each uploaded file is saved per CF's uploadFile
        // semantics (nameconflict handling, result struct with the 23 keys).
        std::string filefield = attrStr(attrs, "filefield");
        std::string destDir = destination.empty() ? file : destination;
        bool strict = attrBool(attrs, "strict", true);
        (void)strict;

        const auto &files = webstrada::UploadRegistry::instance().files();
        const UploadedFile *found = nullptr;
        std::string fieldLow;
        for (char c : filefield) fieldLow += static_cast<char>(tolower((unsigned char)c));
        for (const auto &f : files) {
            std::string fn = f.fieldName;
            std::transform(fn.begin(), fn.end(), fn.begin(),
                           [](unsigned char c) { return (char)tolower(c); });
            if (fn == fieldLow) { found = &f; break; }
        }
        if (!found) {
            throwApp("The form field " + filefield + " did not contain a file.");
        }

        std::string clientName = std::filesystem::path(found->filename).filename().string();
        std::string clientExt;
        std::string clientBase = clientName;
        size_t dot = clientName.find_last_of('.');
        if (dot != std::string::npos) {
            clientExt = clientName.substr(dot + 1);
            clientBase = clientName.substr(0, dot);
        }

        // destination semantics: if it ends with '/' or is an existing
        // directory, the file lands inside with the client name; otherwise the
        // destination is the full target path.
        std::string serverPath;
        std::error_code ec;
        if (std::filesystem::is_directory(destDir, ec)) {
            serverPath = (std::filesystem::path(destDir) / clientName).string();
        } else if (!destDir.empty() && (destDir.back() == '/' || destDir.back() == '\\')) {
            serverPath = (std::filesystem::path(destDir) / clientName).string();
        } else {
            serverPath = destDir;
        }
        std::string serverFile = std::filesystem::path(serverPath).filename().string();
        std::string serverDir = std::filesystem::path(serverPath).parent_path().string();
        std::string attempted = serverFile;
        std::string serverExt;
        std::string serverBase = serverFile;
        size_t sdot = serverFile.find_last_of('.');
        if (sdot != std::string::npos) {
            serverExt = serverFile.substr(sdot + 1);
            serverBase = serverFile.substr(0, sdot);
        }
        bool fileExisted = std::filesystem::exists(serverPath);
        bool overWrite = false, makeUnique = false, skip = false;
        std::string conflict;
        for (char c : nameconflict.empty() ? std::string("error") : nameconflict)
            conflict += static_cast<char>(tolower((unsigned char)c));
        if (fileExisted) {
            if (conflict.empty() || conflict == "error") {
                throwApp("File already exists on server. " + serverPath,
                         "The file " + serverPath + " already exists. The nameconflict attribute is set to 'error'.");
            }
            overWrite = conflict == "overwrite";
            skip = conflict == "skip";
            makeUnique = conflict == "makeunique";
        }
        if (makeUnique) {
            std::string base = serverBase, ext = serverExt;
            std::string candidate = serverPath;
            int num = 0;
            while (std::filesystem::exists(candidate)) {
                num++;
                candidate = (std::filesystem::path(serverDir) /
                             (base + std::to_string(num) + (ext.empty() ? "" : "." + ext))).string();
            }
            serverPath = candidate;
            serverFile = std::filesystem::path(candidate).filename().string();
            serverBase = serverFile;
            size_t sd2 = serverFile.find_last_of('.');
            if (sd2 != std::string::npos) serverExt = serverFile.substr(sd2 + 1);
            else serverExt = "";
        }
        bool wasSaved = false;
        if (!skip) {
            if (!std::filesystem::exists(serverDir)) {
                throwApp("The specified directory " + serverDir + " does not exist.");
            }
            std::ofstream out(serverPath, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(found->content.data()),
                      static_cast<std::streamsize>(found->content.size()));
            out.close();
            wasSaved = true;
        }

        long long lastModified = 0;
        long long fileLength = (long long)found->content.size();
        struct stat stbuf;
        if (::stat(serverPath.c_str(), &stbuf) == 0) {
            lastModified = (long long)stbuf.st_mtime;
        }

        std::string mimeType = found->contentType.empty() ? "application/octet-stream" : found->contentType;
        std::string contentType, contentSubType;
        size_t slash = mimeType.find('/');
        if (slash != std::string::npos) {
            contentType = mimeType.substr(0, slash);
            contentSubType = mimeType.substr(slash + 1);
        } else {
            contentType = mimeType;
            contentSubType = mimeType;
        }

        cfvariant res(cfvariant::Struct);
        res.structSet("ATTEMPTEDSERVERFILE", cfvariant(attempted.c_str()));
        res.structSet("CLIENTDIRECTORY", cfvariant(""));
        res.structSet("CLIENTFILE", cfvariant(clientName.c_str()));
        res.structSet("CLIENTFILEEXT", cfvariant(clientExt.c_str()));
        res.structSet("CLIENTFILENAME", cfvariant(clientBase.c_str()));
        res.structSet("CONTENTSUBTYPE", cfvariant(contentSubType.c_str()));
        res.structSet("CONTENTTYPE", cfvariant(contentType.c_str()));
        res.structSet("FILEEXISTED", boolVar(fileExisted));
        res.structSet("FILESIZE", cfvariant((int)fileLength));
        res.structSet("FILEWASAPPENDED", boolVar(false));
        res.structSet("FILEWASOVERWRITTEN", boolVar(overWrite));
        res.structSet("FILEWASRENAMED", boolVar(makeUnique));
        res.structSet("FILEWASSAVED", boolVar(wasSaved));
        res.structSet("OLDFILESIZE", cfvariant((int)fileLength));
        if (lastModified > 0) {
            cfvariant dt(cfvariant::DateTime);
            dt.m_double = lastModified / 86400.0;
            dt.m_odbcStyle = 1;   // {d 'yyyy-mm-dd'} like CF's OleDate
            res.structSet("DATELASTACCESSED", dt);
        } else {
            res.structSet("DATELASTACCESSED", cfvariant(""));
        }
        res.structSet("SERVERDIRECTORY", cfvariant(serverDir.c_str()));
        res.structSet("SERVERFILE", cfvariant(serverFile.c_str()));
        res.structSet("SERVERFILEEXT", cfvariant(serverExt.c_str()));
        res.structSet("SERVERFILENAME", cfvariant(serverBase.c_str()));
        if (lastModified > 0) {
            cfvariant dt2(cfvariant::DateTime);
            dt2.m_double = lastModified / 86400.0;
            res.structSet("TIMECREATED", dt2);
            cfvariant dt3(cfvariant::DateTime);
            dt3.m_double = lastModified / 86400.0;
            res.structSet("TIMELASTMODIFIED", dt3);
        } else {
            res.structSet("TIMECREATED", cfvariant(""));
            res.structSet("TIMELASTMODIFIED", cfvariant(""));
        }
        res.structSet("FILEATTRIBSET", boolVar(false));
        res.structSet("FILEMODESET", boolVar(false));

        std::string resultVar = result.empty() ? "cffile" : result;
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         resultVar.c_str(), &res);
        return;
    }

    if (lowerAction == "uploadall") {
        // FileUploadAll equivalent: save every uploaded file.
        const auto &files = webstrada::UploadRegistry::instance().files();
        std::string destDir = destination.empty() ? file : destination;
        std::vector<cfvariant> results;
        for (const auto &f : files) {
            std::string clientName = std::filesystem::path(f.filename).filename().string();
            std::string serverPath;
            std::error_code ec;
            if (std::filesystem::is_directory(destDir, ec)) {
                serverPath = (std::filesystem::path(destDir) / clientName).string();
            } else if (!destDir.empty() && (destDir.back() == '/' || destDir.back() == '\\')) {
                serverPath = (std::filesystem::path(destDir) / clientName).string();
            } else {
                serverPath = destDir;
            }
            std::ofstream out(serverPath, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(f.content.data()),
                      static_cast<std::streamsize>(f.content.size()));
            out.close();
            cfvariant r(cfvariant::Struct);
            r.structSet("SERVERFILE", cfvariant(std::filesystem::path(serverPath).filename().string().c_str()));
            r.structSet("CLIENTFILE", cfvariant(clientName.c_str()));
            r.structSet("FILEWASSAVED", boolVar(true));
            results.push_back(r);
        }
        cfvariant arr(cfvariant::Array);
        arr.m_array = new std::vector<cfvariant>(results);
        std::string resultVar = result.empty() ? "cffile" : result;
        cfvariant_assign(static_cast<const cfvariant*>(cgi), static_cast<const cfvariant*>(server),
                         static_cast<const cfvariant*>(cookie), static_cast<const cfvariant*>(application),
                         static_cast<const cfvariant*>(session), static_cast<const cfvariant*>(url),
                         static_cast<const cfvariant*>(form), static_cast<cfvariant*>(variables),
                         resultVar.c_str(), &arr);
        return;
    }

    // Unknown action (validated at compile time for static literals; this
    // catch-all handles dynamic values at run time).
    throwApp("Attribute validation error for CFFILE.",
             "The value of the ACTION attribute, which is currently " + action +
             ", must be one of the values: MOVE,READ,RENAME,UPLOAD,COPY,UPLOADALL,"
             "READBINARY,DELETE,WRITE,APPEND.");
}

} // namespace cfml

/**
 * @file fn_fileuploadall.cpp
 * @brief CFML fileuploadall() built-in.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/upload.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

using webstrada::UploadedFile;

namespace cfml {

static bool allowedExtensionsMatches(const webstrada::UploadedFile &file, const webstrada::string &extList) {
    if (extList.isEmpty()) return true;

    std::string ext = fileExtensionLower(file.filename);
    auto items = extList.split(',', false);
    for (const auto &itemRaw : items) {
        std::string item = normalizeAllowItem(itemRaw);
        if (item.empty()) continue;
        if (item == "*") return true;
        if (item[0] == '.') item = item.substr(1);
        if (!ext.empty() && item == ext) return true;
    }
    return false;
}

cfvariant *cf_fileuploadall(const cfvariant *variables, const cfvariant *dest,
                            const cfvariant *mimeType, const cfvariant *onConflict,
                            const cfvariant *strict, const cfvariant *continueOnError,
                            const cfvariant *errorVariable, const cfvariant *allowedExtensions) {
    if (!variables || !dest) throw webstrada::exception("FileUploadAll: Missing argument(s)");
    if (variables->m_type != cfvariant::Struct) {
        throw webstrada::exception("FileUploadAll: Invalid variables scope");
    }
    string destStr = const_cast<cfvariant*>(dest)->toString();

    std::string serverDir, fileBase;
    bool dirMode = true;
    resolveUploadDestination(destStr, "FileUploadAll", serverDir, dirMode, fileBase);

    string mimeList;
    if (mimeType) mimeList = const_cast<cfvariant*>(mimeType)->toString();
    string extList;
    if (allowedExtensions) extList = const_cast<cfvariant*>(allowedExtensions)->toString();
    string conflict = "error";
    if (onConflict) conflict = const_cast<cfvariant*>(onConflict)->toString();
    conflict.toLower();
    bool contOnError = continueOnError ? isTruthy(*continueOnError) : false;
    string errVar = "cffile.uploadAllErrors";
    if (errorVariable) {
        string ev = const_cast<cfvariant*>(errorVariable)->toString();
        if (!ev.isEmpty()) errVar = ev;
    }

    const auto &files = webstrada::UploadRegistry::instance().files();
    if (files.empty()) {
        throw webstrada::exception("FileUploadAll: No files were uploaded.");
    }

    auto *ret = new cfvariant(cfvariant::Array);
    auto *errors = new cfvariant(cfvariant::Array);

    for (const auto &file : files) {
        try {
            if (!mimeList.isEmpty() && !mimeTypeMatches(file, mimeList)) {
                throw webstrada::exception("FileUploadAll: The uploaded file " + string(file.filename.c_str()) + " is not of an accepted type. Accepted MIME types or extensions: " + mimeList);
            }
            if (!extList.isEmpty() && !allowedExtensionsMatches(file, extList)) {
                throw webstrada::exception("FileUploadAll: The uploaded file " + string(file.filename.c_str()) + " is not of an accepted extension. Accepted extensions: " + extList);
            }
            cfvariant *result = saveUploadedFile(file, serverDir, conflict, fileBase);
            cf_register_temp(result);
            ret->insert(*result);
        } catch (const webstrada::exception &ex) {
            if (!contOnError) {
                delete errors;
                delete ret;
                throw;
            }
            // Record the failure into the error variable and keep going.
            cfvariant errStruct = cfvariant::Struct;
            errStruct.set("REASON") = cfvariant("ERROR");
            errStruct.set("MESSAGE") = ex.m_message;
            errStruct.set("DETAIL") = ex.m_detail;
            errStruct.set("CLIENTFILE") = cfvariant(file.filename.c_str());
            std::string cname, cext;
            splitNameExt(file.filename, cname, cext);
            errStruct.set("CLIENTFILENAME") = cfvariant(cname.c_str());
            errStruct.set("CLIENTFILEEXT") = cfvariant(cext.c_str());
            errStruct.set("DEST") = cfvariant(serverDir.c_str());
            errors->insert(errStruct);
        }
    }

    if (contOnError) {
        auto *vars = const_cast<cfvariant*>(variables);
        vars->set(errVar) = *errors;
    }
    delete errors;

    return ret;
}

} // namespace cfml

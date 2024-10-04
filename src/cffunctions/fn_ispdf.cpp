/**
 * @file fn_ispdf.cpp
 * @brief CFML ispdffile() / ispdfarchive() / ispdfobject() built-ins.
 *
 * CF 2025 routes these to the PDF service (ServiceFactory.getPDFService()):
 * IsPDFFile reads the file and returns whether it is a valid PDF; IsPDFArchive
 * validates PDF/A conformance; IsPDFObject reports whether a value is a PDF
 * object. The RDS host has no PDF service (always NO, see BUGS_CF.md), so the
 * engine sniffs PDF magic bytes / the PDF/A Info entry per the documented
 * behavior.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <cctype>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace cfml {

namespace {

bool readFileBytesPath(const std::string &path, std::vector<char> &out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    out.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return true;
}

// True when the buffer looks like a PDF: starts with "%PDF-" and contains the
// "%%EOF" trailer marker.
bool looksLikePdf(const std::vector<char> &data) {
    if (data.size() < 8) return false;
    if (std::strncmp(data.data(), "%PDF-", 5) != 0) return false;
    std::string s(data.data(), data.size());
    return s.find("%%EOF") != std::string::npos;
}

} // namespace

cfvariant *cf_ispdffile(const cfvariant *value) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    if (!value) return ret;
    // Binary PDF data.
    if (value->m_type == cfvariant::Binary && value->m_binary) {
        ret->m_bool = looksLikePdf(std::vector<char>(
            reinterpret_cast<const char*>(value->m_binary->data()),
            reinterpret_cast<const char*>(value->m_binary->data() + value->m_binary->size())));
        return ret;
    }
    // A file path string.
    if (value->m_type == cfvariant::String) {
        webstrada::string path = const_cast<cfvariant*>(value)->toString();
        std::vector<char> data;
        if (readFileBytesPath(path.constData() ? path.constData() : "", data)) {
            ret->m_bool = looksLikePdf(data);
        }
    }
    return ret;
}

cfvariant *cf_ispdfarchive(const cfvariant *value, const cfvariant *standard) {
    (void)standard;
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    if (!value) return ret;
    // A PDF/A archive is a PDF with a document-level Info entry naming a PDF/A
    // conformance version. Sniff the "GTS_PDFA1" (or PDF/A) key in the catalog
    // /Metadata. For the common case we accept any valid PDF with a PDF/A
    // marker in its text; without the marker it is not an archive.
    std::vector<char> data;
    if (value->m_type == cfvariant::Binary && value->m_binary) {
        data.assign(reinterpret_cast<const char*>(value->m_binary->data()),
                    reinterpret_cast<const char*>(value->m_binary->data() + value->m_binary->size()));
    } else if (value->m_type == cfvariant::String) {
        webstrada::string path = const_cast<cfvariant*>(value)->toString();
        readFileBytesPath(path.constData() ? path.constData() : "", data);
    }
    if (!looksLikePdf(data)) return ret;
    std::string s(data.data(), data.size());
    // PDF/A markers used by the generators: GTS_PDFA1, pdfaid, Part 1/2/3.
    bool hasMarker = s.find("GTS_PDFA1") != std::string::npos ||
                     s.find("pdfaid") != std::string::npos ||
                     s.find("PDF/A") != std::string::npos;
    ret->m_bool = hasMarker;
    return ret;
}

cfvariant *cf_ispdfobject(const cfvariant *value) {
    auto *ret = new cfvariant(cfvariant::Boolean);
    ret->m_bool = false;
    if (!value) return ret;
    // A PDF object is a struct produced by pdf action="read" / GetPDFInfo.
    // The engine has no PDF reader, so only a Binary/struct PDF payload with
    // the PDF magic is recognized.
    if (value->m_type == cfvariant::Binary && value->m_binary) {
        ret->m_bool = looksLikePdf(std::vector<char>(
            reinterpret_cast<const char*>(value->m_binary->data()),
            reinterpret_cast<const char*>(value->m_binary->data() + value->m_binary->size())));
    }
    return ret;
}

} // namespace cfml

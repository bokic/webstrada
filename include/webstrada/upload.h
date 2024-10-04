#pragma once

#include <vector>
#include <string>
#include <cstddef>

namespace webstrada {

// A single file received in a multipart/form-data request body.
struct UploadedFile {
    std::string fieldName;    // form field name (case preserved)
    std::string filename;     // basename from Content-Disposition
    std::vector<std::byte> content;
    std::string contentType;  // e.g. "text/plain", may be empty
};

// Request-scoped storage of uploaded files. The appserver worker fills it
// after parsing a multipart/form-data body, and the FileUpload/FileUploadAll
// runtime functions read from it. Thread-local so concurrent workers do not
// leak files between requests.
class UploadRegistry {
public:
    static UploadRegistry &instance();
    void setFiles(std::vector<UploadedFile> &&files);
    const std::vector<UploadedFile> &files() const;
private:
    static thread_local std::vector<UploadedFile> s_files;
};

}

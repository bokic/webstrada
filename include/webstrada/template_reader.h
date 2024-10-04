#pragma once

#include <string>
#include <vector>

namespace webstrada {

// Result of reading and decoding a CFML template source into the engine's
// internal UTF-8 representation (cfml_text). Mirrors what ColdFusion's
// TemplateReader produces: the source is always decoded to UTF-8 before it is
// handed to the textparser.
struct TemplateReadResult {
    std::string utf8Text;          // decoded template source (UTF-8)
    std::string usedEncoding;      // CF-style canonical name of the charset the
                                   // source was read with (see encodingIsMatch)
    std::string bomEncoding;       // BOM charset name ("UTF8" /
                                   // "UnicodeLittleUnmarked" /
                                   // "UnicodeBigUnmarked"), empty when no BOM
    bool explicitEncoding = false; // a BOM was present
};

// Reads the file at `path` and decodes it following ColdFusion's input-encoding
// order (BOM -> cfprocessingdirective pageEncoding -> ICU charset detection ->
// default input charset). Throws webstrada::exception with CF's exact messages
// for a BOM/pageEncoding conflict or an unsupported pageEncoding name.
TemplateReadResult readTemplateFile(const std::string &path);

// Same as readTemplateFile for an in-memory byte buffer (stdin / unit tests).
// `filename` is optional and only affects directive detection for .cfc script
// components (the grammar override needs the extension).
TemplateReadResult readTemplateBuffer(const std::vector<char> &bytes, const std::string &filename = "");

// CF BOMReader.isEncodingMatch: true when the pageEncoding value `desired`
// names the same charset as the canonical `usedEncoding` the source was read
// with. Throws the CF "The specified page encoding, X, is not supported."
// message for a name no supported charset matches.
bool encodingIsMatch(const std::string &desired, const std::string &usedEncoding);

// Returns CF's compile-time error message for an unsupported pageEncoding name
// (empty string when the name is a supported charset). An empty value reports
// '' exactly like CF ("The specified page encoding, '', is not supported.").
std::string pageEncodingError(const std::string &name);

} // namespace webstrada

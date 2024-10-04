#pragma once

// Shared OWASP-ESAPI-compatible encoder/decoder infrastructure used by the
// EncodeFor* / DecodeForHTML / Canonicalize family (see
// CFFUNCTION_IMPLEMENTATION_ACTION_PLAN.md Tier 2). Algorithms replicate the
// ESAPI 2.x DefaultEncoder + codecs (HTMLEntityCodec, XMLEntityCodec,
// JavaScriptCodec, CSSCodec, PercentCodec) byte-for-byte, verified against
// ColdFusion 2025 on the RDS host.

#include <webstrada/string.h>
#include <string>

namespace cfml {

// Decodes the UTF-8 code point starting at in[i]; advances i past the
// sequence. Returns the code point (bytes are copied verbatim into cpBytes for
// malformed input handled by callers that need raw bytes).
int esapiUtf8Next(const std::string &in, size_t &i);

// True when `ch` is ASCII alphanumeric (a-z A-Z 0-9).
bool esapiIsAlphanumeric(int ch);

// HTMLEntityCodec.encodeCharacter with the given immune-set. Returns the
// encoded text for one Unicode code point.
std::string esapiEncodeHtmlCodePoint(int codePoint, const char *immune, int immuneLen);

// Encodes a whole UTF-8 string using a per-code-point encoder function.
std::string esapiEncodeString(const std::string &in,
                              std::string (*enc)(int, const char *, int),
                              const char *immune, int immuneLen);

// XMLEntityCodec.encodeCharacter.
std::string esapiEncodeXmlCodePoint(int codePoint, const char *immune, int immuneLen);

// JavaScriptCodec.encodeCharacter (uppercase \xHH / \uHHHH).
std::string esapiEncodeJavaScriptCodePoint(int codePoint, const char *immune, int immuneLen);

// CSSCodec.encodeCharacter (lowercase \HH + space).
std::string esapiEncodeCssCodePoint(int codePoint, const char *immune, int immuneLen);

// HTMLEntityCodec.decode / XMLEntityCodec.decode / PercentCodec.decode /
// JavaScriptCodec.decode — used by DecodeForHTML and the Canonicalize loop.
std::string esapiDecodeHtml(const std::string &input);
std::string esapiDecodeXml(const std::string &input);
std::string esapiDecodePercent(const std::string &input);
std::string esapiDecodeJavaScript(const std::string &input);

// Canonicalize loop shared with Canonicalize(). Mirrors
// DefaultEncoder.canonicalize(input, restrictMultiple, restrictMixed) and
// throws webstrada::exception("Input validation failure") with type
// "org.owasp.esapi.errors.IntrusionException" when a restriction trips.
std::string esapiCanonicalize(const std::string &input,
                              bool restrictMultiple, bool restrictMixed);

// ESAPIUtils-style wrapper: on a thrown IntrusionException, returns "" when
// throwOnError is false, else rethrows (Canonicalize()'s fourth arg).
std::string esapiCanonicalizeCatch(const std::string &input,
                                   bool restrictMultiple, bool restrictMixed,
                                   bool throwOnError);

// DefaultEncoder.encodeForDN / encodeForLDAP(input, true).
std::string esapiEncodeDn(const std::string &input);
std::string esapiEncodeLdap(const std::string &input);

} // namespace cfml

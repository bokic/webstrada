/**
 * @file template_reader.cpp
 * @brief Template source reading + input-encoding detection.
 *
 * Replicates ColdFusion 2025's TemplateReader / NeoTranslator /
 * CFMLParserBase.verifyPageEncoding input-encoding order for a CFML source:
 *
 *   1. A UTF BOM (UTF-8 EF BB BF / UTF-16LE FF FE / UTF-16BE FE FF) is
 *      authoritative; a <cfprocessingdirective pageEncoding> that does not
 *      match the BOM is a hard compile error ("Cannot use the charset X
 *      because the file has a Byte Order Mark indicating it uses the charset
 *      Y."). The conflict check runs after the name-support check, so a BOM'd
 *      file with an unknown pageEncoding reports "The specified page encoding,
 *      X, is not supported." exactly like CF.
 *   2. Without a BOM, a cfprocessingdirective pageEncoding directive (tag form
 *      or the `pageencoding "..";` statement inside a script component body)
 *      re-reads the file with the named charset, overriding detection and the
 *      default. Matching is CF's BOMReader.isEncodingMatch (canonical-name
 *      comparison with the utf16/utf-16 <-> Unicode{Big,Little}Unmarked rule).
 *   3. Without a directive, ICU charset detection (ucsdet, input filter on,
 *      confidence >= config::charsetDetectionMinConfidence, default 100) picks
 *      the charset.
 *   4. Fallback: the server default input charset (config::defaultInputCharset,
 *      default UTF-8).
 *
 * The source is always decoded to UTF-8 (the engine's internal string
 * encoding) and handed to the textparser via textparser_openmem.
 */

#include <webstrada/template_reader.h>

#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <textparser.hpp>
#include <cfml_definition.json.h>

#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

using namespace webstrada;

namespace webstrada {

namespace {

// ---------------------------------------------------------------------------
// BOM detection
// ---------------------------------------------------------------------------

const char kUtf8Bom[] = "\xEF\xBB\xBF";     // 3 bytes
const char kUtf16LeBom[] = "\xFF\xFE";      // 2 bytes
const char kUtf16BeBom[] = "\xFE\xFF";      // 2 bytes

// Returns the CF canonical BOM encoding name if the buffer starts with a UTF
// BOM (and advances *bomLen past it); "" otherwise. Order matches CF's
// BOMReader.getBOMEncoding (UTF-8, then UTF-16LE, then UTF-16BE).
std::string detectBom(const std::vector<char> &bytes, size_t &bomLen)
{
    if (bytes.size() >= 3 && std::memcmp(bytes.data(), kUtf8Bom, 3) == 0) {
        bomLen = 3;
        return "UTF8";
    }
    if (bytes.size() >= 2 && std::memcmp(bytes.data(), kUtf16LeBom, 2) == 0) {
        bomLen = 2;
        return "UnicodeLittleUnmarked";
    }
    if (bytes.size() >= 2 && std::memcmp(bytes.data(), kUtf16BeBom, 2) == 0) {
        bomLen = 2;
        return "UnicodeBigUnmarked";
    }
    bomLen = 0;
    return "";
}

// ---------------------------------------------------------------------------
// ICU helpers
// ---------------------------------------------------------------------------

std::string lowerAscii(const std::string &s)
{
    std::string r = s;
    for (auto &c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

std::string upperAscii(const std::string &s)
{
    std::string r = s;
    for (auto &c : r) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
}

// ICU can open a converter for `name`?
bool charsetSupported(const std::string &name)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = ucnv_open(name.c_str(), &status);
    if (U_FAILURE(status)) return false;
    ucnv_close(conv);
    return true;
}

// Map a CF-style canonical/alias name to the ICU converter name used to decode.
std::string icuConverterName(const std::string &name)
{
    std::string up = upperAscii(name);
    for (auto &c : up) if (c == '_') c = '-';
    std::string noDash;
    for (auto c : up) if (c != '-') noDash.push_back(c);
    if (noDash == "UTF8") return "UTF-8";
    if (noDash == "UTF16LE" || name == "UnicodeLittleUnmarked") return "UTF-16LE";
    if (noDash == "UTF16BE" || name == "UnicodeBigUnmarked") return "UTF-16BE";
    return name;
}

// Decode `bytes` (already BOM-stripped) from `charset` to a UTF-8 std::string.
// Unmappable bytes decode to the ICU substitute character (U+FFFD), matching
// the JDK decoder replacement behavior. An unknown converter throws CF's
// "The specified page encoding, X, is not supported." message.
std::string decodeToUtf8(const std::vector<char> &bytes, const std::string &charset)
{
    if (bytes.empty()) return "";
    const std::string convName = icuConverterName(charset);
    int32_t cap = static_cast<int32_t>(bytes.size()) * 4 + 16;
    std::string out;
    out.resize(static_cast<size_t>(cap));
    for (;;) {
        UErrorCode status = U_ZERO_ERROR;
        int32_t len = ucnv_convert("UTF-8", convName.c_str(),
                                   out.data(), cap,
                                   bytes.data(), static_cast<int32_t>(bytes.size()), &status);
        if (status == U_BUFFER_OVERFLOW_ERROR) {
            cap *= 2;
            out.resize(static_cast<size_t>(cap));
            continue;
        }
        if (U_FAILURE(status)) {
            throw webstrada::exception(
                ("The specified page encoding, " + charset + ", is not supported.").c_str());
        }
        out.resize(static_cast<size_t>(len));
        return out;
    }
}

// ICU charset detection with CF's EncodingDetector settings (input filter on,
// no declared encoding). Returns the detected name when its confidence is >=
// config::charsetDetectionMinConfidence and a converter exists, else "".
std::string detectCharset(const std::vector<char> &bytes)
{
    if (bytes.empty()) return "";
    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector *det = ucsdet_open(&status);
    if (U_FAILURE(status)) return "";
    ucsdet_enableInputFilter(det, true);
    ucsdet_setText(det, bytes.data(), static_cast<int32_t>(bytes.size()), &status);
    if (U_FAILURE(status)) {
        ucsdet_close(det);
        return "";
    }
    const UCharsetMatch *match = ucsdet_detect(det, &status);
    std::string result;
    if (U_SUCCESS(status) && match != nullptr) {
        int32_t conf = ucsdet_getConfidence(match, &status);
        const char *name = ucsdet_getName(match, &status);
        if (U_SUCCESS(status) && name != nullptr &&
            conf >= config::charsetDetectionMinConfidence &&
            charsetSupported(name)) {
            result = name;
        }
    }
    ucsdet_close(det);
    return result;
}

// ---------------------------------------------------------------------------
// CF charset-name canonicalization (BOMReader.getCanonicalEncoding equivalent)
// ---------------------------------------------------------------------------

// Canonicalizes a charset name the way CF's isEncodingMatch compares them.
// Mirrors Java's InputStreamReader.getEncoding() for the names CF templates
// use: the UTF-8 family canonicalizes to "UTF8", UTF-16LE/BE to
// "UnicodeLittleUnmarked"/"UnicodeBigUnmarked", UTF-16 to "UTF-16", and the
// common single-byte aliases to their Java canonical names. Any other name is
// kept verbatim (case-preserved) once ICU confirms it is a supported
// converter. Unknown names throw CF's message.
std::string javaCanonicalName(const std::string &name)
{
    std::string up = upperAscii(name);
    std::string noDash;
    for (auto c : up) if (c != '-' && c != '_') noDash.push_back(c);

    if (noDash == "UTF8") return "UTF8";
    if (noDash == "UTF16LE") return "UnicodeLittleUnmarked";
    if (noDash == "UTF16BE") return "UnicodeBigUnmarked";
    if (noDash == "UTF16") return "UTF-16";
    if (noDash == "ISO88591" || noDash == "LATIN1" || noDash == "88591") return "ISO-8859-1";
    if (noDash == "USASCII" || noDash == "ASCII" || noDash == "ISO646US") return "US-ASCII";
    if (noDash == "CP1252" || noDash == "WINDOWS1252") return "windows-1252";

    if (name.empty()) {
        throw webstrada::exception("The specified page encoding, '', is not supported.");
    }
    if (!charsetSupported(name)) {
        throw webstrada::exception(("The specified page encoding, " + name + ", is not supported.").c_str());
    }
    return name;
}

bool isUtf16Name(const std::string &name)
{
    std::string low = lowerAscii(name);
    return low == "utf16" || low == "utf-16";
}

// ---------------------------------------------------------------------------
// cfprocessingdirective pageEncoding scan
// ---------------------------------------------------------------------------

// Extracts the value of a `pageencoding` attribute from a tag's raw text
// (handles single/double quotes and unquoted values). Returns std::nullopt
// when the attribute is absent, an empty string when present-but-empty (CF
// treats pageEncoding="" as an error naming '').
std::optional<std::string> extractPageEncoding(const std::string &tagText)
{
    // Find `pageencoding` (case-insensitive) as a word.
    std::string low = lowerAscii(tagText);
    size_t pos = 0;
    while ((pos = low.find("pageencoding", pos)) != std::string::npos) {
        size_t before = pos;
        size_t after = pos + 12;
        bool wordStart = (before == 0) ||
            !(std::isalnum(static_cast<unsigned char>(low[before - 1])) || low[before - 1] == '_');
        bool wordEnd = (after >= low.size()) ||
            !(std::isalnum(static_cast<unsigned char>(low[after])) || low[after] == '_');
        if (wordStart && wordEnd) {
            size_t i = after;
            while (i < tagText.size() &&
                   (tagText[i] == '=' || tagText[i] == ' ' || tagText[i] == '\t')) i++;
            if (i >= tagText.size()) return std::nullopt;
            if (tagText[i] == '"' || tagText[i] == '\'') {
                char q = tagText[i];
                size_t vs = i + 1;
                size_t ve = tagText.find(q, vs);
                if (ve == std::string::npos) return std::nullopt;
                return std::string(tagText.substr(vs, ve - vs));
            }
            // Unquoted: read to whitespace or '>'.
            size_t vs = i;
            size_t ve = vs;
            while (ve < tagText.size() && tagText[ve] != ' ' && tagText[ve] != '\t' &&
                   tagText[ve] != '\r' && tagText[ve] != '\n' && tagText[ve] != '>') ve++;
            return std::string(tagText.substr(vs, ve - vs));
        }
        pos = after;
    }
    return std::nullopt;
}

// Slices a token's raw text out of the source, clamped to the buffer bounds.
std::string sliceToken(const textparser_token_item *t, const std::string &text)
{
    size_t pos = textparser_get_token_position(t);
    if (pos >= text.size()) return "";
    size_t len = std::min<size_t>(t->len, text.size() - pos);
    return text.substr(pos, len);
}

// Recursively walks the textparser token tree, collecting the pageEncoding
// values (and source lines) of every <cfprocessingdirective ...> start tag
// (tag form) and every `pageencoding <literal>;` statement (script component
// form, tokenized as a Variable followed by a string literal). Comment/string
// tokens are never mistaken for the directive, so a commented-out directive or
// a string containing the word `pageencoding` is ignored like CF. The line of a
// directive is taken from the textparser line map (built on `handle`), which is
// encoding-independent, so multibyte UTF-8 content before the directive cannot
// shift it.
void scanDirectiveTokens(const textparser_token_item *item, const std::string &text,
                         textparser_t handle,
                         std::vector<std::pair<std::string, size_t>> &out)
{
    for (const textparser_token_item *t = item; t != nullptr; t = t->next) {
        if (t->token_id == TextParser_cfml_StartTag) {
            std::string tagText = sliceToken(t, text);
            std::string low = lowerAscii(tagText);
            if (low.find("<cfprocessingdirective") == 0) {
                std::optional<std::string> pe = extractPageEncoding(tagText);
                if (pe.has_value()) {
                    out.emplace_back(*pe,
                        textparser_get_line_number_at_position(handle, textparser_get_token_position(t)) + 1);
                }
            }
        } else if (t->token_id == TextParser_cfml_Variable) {
            std::string varText = sliceToken(t, text);
            if (lowerAscii(varText) == "pageencoding") {
                const textparser_token_item *sib = t->next;
                if (sib != nullptr &&
                    (sib->token_id == TextParser_cfml_SingleString ||
                     sib->token_id == TextParser_cfml_DoubleString)) {
                    std::string lit = sliceToken(sib, text);
                    if (lit.size() >= 2 &&
                        (lit.front() == '"' || lit.front() == '\'') &&
                        lit.back() == lit.front()) {
                        out.emplace_back(lit.substr(1, lit.size() - 2),
                            textparser_get_line_number_at_position(handle, textparser_get_token_position(t)) + 1);
                    }
                }
            }
        }
        if (t->child) {
            scanDirectiveTokens(t->child, text, handle, out);
        }
    }
}

// Collects the literal pageEncoding values (with source line numbers) of every
// cfprocessingdirective in `text` (both forms), in document order. `filename`
// (optional) is passed to the textparser so the .cfc script-component grammar
// override applies to the tokenization.
std::vector<std::pair<std::string, size_t>> findPageEncodingDirectives(const std::string &text,
                                                                       const std::string &filename)
{
    std::vector<std::pair<std::string, size_t>> out;
    if (lowerAscii(text).find("pageencoding") == std::string::npos) {
        return out;
    }
    textparser_t handle = nullptr;
    if (textparser_openmem(text.data(), static_cast<int>(text.size()),
                           TEXTPARSER_ENCODING_UTF_8, &handle) != 0) {
        return out;
    }
    if (!filename.empty()) {
        textparser_set_filename(handle, filename.c_str());
    }
    if (textparser_parse(handle, &cfml_definition) != 0) {
        textparser_close(handle);
        return out;
    }
    textparser_build_line_map(handle);
    const textparser_token_item *first = textparser_get_first_token(handle);
    if (first) scanDirectiveTokens(first, text, handle, out);
    textparser_close(handle);
    return out;
}

} // namespace

bool encodingIsMatch(const std::string &desired, const std::string &usedEncoding)
{
    // Canonicalizing the desired name validates it (throws CF's message for an
    // unknown charset), matching CF's getCanonicalEncoding-inside-isEncodingMatch.
    std::string desiredCanonical = javaCanonicalName(desired);
    if (desiredCanonical == javaCanonicalName(usedEncoding)) return true;
    if (isUtf16Name(desired)) {
        std::string used = javaCanonicalName(usedEncoding);
        if (used == "UnicodeLittleUnmarked" || used == "UnicodeBigUnmarked") return true;
    }
    return false;
}

std::string pageEncodingError(const std::string &name)
{
    try {
        javaCanonicalName(name);
        return "";
    } catch (const webstrada::exception &ex) {
        return ex.m_message.isEmpty() ? "" : std::string(ex.m_message.constData());
    }
}

TemplateReadResult readTemplateBuffer(const std::vector<char> &bytes, const std::string &filename)
{
    TemplateReadResult result;

    // 1. BOM.
    size_t bomLen = 0;
    std::string bom = detectBom(bytes, bomLen);
    std::vector<char> body(bytes.begin() + static_cast<long>(bomLen), bytes.end());
    if (!bom.empty()) {
        result.explicitEncoding = true;
        result.bomEncoding = bom;
    }

    // 2. Provisional charset: BOM, else ICU detection, else the default.
    std::string chosen;
    if (!bom.empty()) {
        chosen = bom;
    } else {
        std::string detected = detectCharset(body);
        if (!detected.empty()) {
            chosen = detected;
        } else {
            chosen = config::defaultInputCharset;
        }
    }
    result.usedEncoding = javaCanonicalName(chosen);

    // 3. Locate cfprocessingdirective pageEncoding directives on a provisional
    //    decode (the directive text is ASCII and survives any ASCII-compatible
    //    provisional decode).
    std::string provisionalText = decodeToUtf8(body, chosen);
    std::vector<std::pair<std::string, size_t>> directives =
        findPageEncodingDirectives(provisionalText, filename);

    // 4. Apply CF's verifyPageEncoding semantics.
    for (const auto &d : directives) {
        const std::string &pe = d.first;
        // A non-literal value (#...# expression) is a compile error naming the
        // directive's line, exactly like CF (the line comes from the textparser
        // line map, so it is encoding-independent).
        if (pe.find('#') != std::string::npos) {
            throw webstrada::exception(
                ("Expression at line " + std::to_string(d.second) +
                 " has to be constant value.").c_str());
        }
        if (encodingIsMatch(pe, result.usedEncoding)) continue;
        if (result.explicitEncoding) {
            // Hard compile error: the BOM is authoritative.
            throw webstrada::exception(
                ("Cannot use the charset " + pe +
                 " because the file has a Byte Order Mark indicating it uses the charset " +
                 result.bomEncoding + ".").c_str());
        }
        // No BOM: re-read the file with the directive's charset (overrides
        // detection and the default).
        chosen = pe;
        result.usedEncoding = javaCanonicalName(pe);
    }

    // 5. Final decode.
    result.utf8Text = decodeToUtf8(body, chosen);
    return result;
}

TemplateReadResult readTemplateFile(const std::string &path)
{
    FILE *f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) {
        throw webstrada::exception(("Error opening file for parsing! " + path).c_str());
    }
    std::vector<char> bytes;
    char buf[8192];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) {
        bytes.insert(bytes.end(), buf, buf + n);
    }
    std::fclose(f);
    return readTemplateBuffer(bytes, path);
}

} // namespace webstrada

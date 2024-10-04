#pragma once

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>
#include <webstrada/cfimage.h>
#include <webstrada/upload.h>
#include "../cftags/common.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <json-c/json.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>
#include <cairo.h>
#include <jpeglib.h>
#include <zlib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/xmlschemas.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <csetjmp>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <functional>
#include <utility>
#include <filesystem>
#include <fstream>

namespace cfml {

struct ReMatchResult {
    PCRE2_SIZE start = 0;
    PCRE2_SIZE end = 0;
    std::vector<std::pair<PCRE2_SIZE, PCRE2_SIZE>> groups;
};

struct CipherAlg {
    enum CipherBase { CipherAES, CipherDES, CipherDESede, CipherBlowfish, CipherUnknown };
    CipherBase base = CipherUnknown;
    webstrada::string mode = "ECB";   // ECB, CBC, CFB, OFB, ...
    bool padding = true;             // PKCS5Padding / NoPadding
    int blockSize = 16;
};

struct CfdumpOptions {
    std::string label;
    int top = -1;        // array: limit rows
    int keys = -1;       // struct: limit keys
    std::string show;    // struct: only these keys (comma separated)
    std::string hide;    // struct: exclude these keys
};

struct MemReader {
    const uint8_t *data;
    size_t len;
    size_t pos;
};

struct JpegErrorMgr {
    jpeg_error_mgr pub;
    jmp_buf jb;
};

struct TiffValue {
    enum Kind {
        NONE, STRING, INT, LONG, RATIONAL, RATIONAL_ARRAY,
        INT_ARRAY, LONG_ARRAY, BYTE_ARRAY, SHORT_ARRAY, FLOAT_ARRAY, DOUBLE_ARRAY
    } kind = NONE;
    std::string str;
    long long i = 0;                 // INT / LONG
    long long num = 0, den = 1;      // RATIONAL
    std::vector<long long> iarr;     // INT_ARRAY / LONG_ARRAY / SHORT_ARRAY
    std::vector<long long> rnum, rden; // RATIONAL_ARRAY
    std::vector<unsigned char> bytes; // BYTE_ARRAY
    std::vector<float> farr;         // FLOAT_ARRAY
    std::vector<double> darr;        // DOUBLE_ARRAY
};

struct TiffTag {
    int tag = 0;
    TiffValue val;
};

struct TiffDir {
    int kind = 0;
    std::vector<TiffTag> tags; // ascending tag order
};

struct TiffBytes {
    const uint8_t *p = nullptr;
    size_t n = 0;
    bool be = false;

    uint16_t u16(size_t off) const {
        return be ? (uint16_t)(((uint16_t)p[off] << 8) | p[off + 1])
                  : (uint16_t)(p[off] | ((uint16_t)p[off + 1] << 8));
    }
    uint32_t u32(size_t off) const {
        if (be) return ((uint32_t)p[off] << 24) | ((uint32_t)p[off + 1] << 16) |
                       ((uint32_t)p[off + 2] << 8) | p[off + 3];
        return (uint32_t)p[off] | ((uint32_t)p[off + 1] << 8) |
               ((uint32_t)p[off + 2] << 16) | ((uint32_t)p[off + 3] << 24);
    }
    uint8_t u8(size_t off) const { return p[off]; }
    int16_t s16(size_t off) const { return (int16_t)u16(off); }
    int32_t s32(size_t off) const { return (int32_t)u32(off); }
    float f32(size_t off) const {
        uint32_t v = u32(off);
        float f;
        memcpy(&f, &v, 4);
        return f;
    }
    double f64(size_t off) const {
        uint64_t v;
        if (be) {
            v = ((uint64_t)u32(off) << 32) | u32(off + 4);
        } else {
            v = u32(off) | ((uint64_t)u32(off + 4) << 32);
        }
        double d;
        memcpy(&d, &v, 8);
        return d;
    }
};

struct IptcEntry {
    int tag = 0;        // record | (dataset << 8)
    std::string str;    // string value
    bool isInt = false; // setInt stored value
    long long i = 0;
    std::vector<std::string> strArray; // repeated tags accumulate
};

struct CaptchaRng {
    uint64_t state;
    explicit CaptchaRng(uint64_t seed) : state(seed ? seed : 0x9E3779B97F4A7C15ULL) {}
    uint32_t next()
    {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return (uint32_t)((state * 2685821657736338717ULL) >> 32);
    }
    int nextInt(int bound) { return bound <= 0 ? 0 : (int)(next() % (uint32_t)bound); }
    float nextFloat() { return (float)(next() & 0xFFFFFF) / 16777216.0f; }
    bool nextBoolean() { return (next() & 1) != 0; }
    double nextDouble() { return (double)((next() >> 6) & 0x1FFFFF) / 2097152.0; }
};

struct CaptchaPoint { int x, y; };

extern thread_local const cfml::LocaleInfo *g_currentLocale;
extern thread_local webstrada::string g_currentLocaleStr;
extern thread_local std::set<const void*> g_serializeVisited;
extern thread_local bool g_cfdump_style_emitted;
extern thread_local bool g_cfdump_abort_pending;
extern thread_local std::string g_cfdump_style_cache;
extern thread_local bool g_cfdump_udf_first_space;
extern thread_local std::set<int> g_cfdump_udf_seen_depths;
extern thread_local bool g_randGenInitialized;
extern const std::string kImageFormats;

string joinList(const std::vector<string> &elements, const string &delim);
string joinListItems(const std::vector<string> &items, const string &delim);
std::vector<string> splitList(const string &listStr, const string &delim, bool includeEmptyFields);
std::vector<string> structOrderedKeys(const cfvariant &st);
webstrada::string escapeHtmlEdit(const webstrada::string &in);
void throwSequenceNotRecognized(char c);
cfvariant queryBuildRowStruct(const QueryData *qd, int rowIndex);
int binaryDecodeHexVal(char c);
void binaryDecodeHex(const webstrada::string &s, std::vector<std::byte> &out);
int binaryDecodeBase64Val(char c, bool urlSafe);
void binaryDecodeBase64(const webstrada::string &s, std::vector<std::byte> &out, bool urlSafe);
void binaryDecodeUU(const webstrada::string &s, std::vector<std::byte> &out);
void binaryEncodeHex(const std::vector<std::byte> &in, webstrada::string &out);
void binaryEncodeBase64(const std::vector<std::byte> &in, webstrada::string &out, bool urlSafe);
void binaryEncodeUU(const std::vector<std::byte> &in, webstrada::string &out);
void variantToBytes(const cfvariant *v, const webstrada::string &encoding, std::vector<std::byte> &out);
const EVP_MD *cryptoDigestByName(const webstrada::string &alg);
webstrada::string uppercaseHex(const unsigned char *data, size_t len);
void loadCryptoProviders();
void decodeEncryptKey(const webstrada::string &key, std::vector<std::byte> &out);
bool stringEqualsNoCase(const webstrada::string &a, const webstrada::string &b);
bool parseCipherAlgorithm(const webstrada::string &algStr, CipherAlg &alg);
const EVP_CIPHER *cipherForAlg(const CipherAlg &alg, const std::vector<std::byte> &key, int &keyLen);
void cipherEncrypt(     const std::vector<std::byte> &input,     const CipherAlg &alg,     const std::vector<std::byte> &key,     const std::vector<std::byte> &iv,     bool prependIv,     std::vector<std::byte> &out);
void cipherDecrypt(     const std::vector<std::byte> &input,     const CipherAlg &alg,     const std::vector<std::byte> &key,     const std::vector<std::byte> &iv,     std::vector<std::byte> &out);
void splitContentType(const std::string &ct, std::string &type, std::string &subtype);
void splitNameExt(const std::string &filename, std::string &name, std::string &ext);
std::string fileExtensionLower(const std::string &filename);
std::string normalizeAllowItem(const webstrada::string &raw);
bool mimeTypeMatches(const webstrada::UploadedFile &file, const webstrada::string &mimeList);
cfvariant *buildUploadStruct(const webstrada::UploadedFile &file,                                     const std::string &serverDir,                                     const std::string &serverFile,                                     const std::string &attemptedServerFile,                                     bool fileExisted,                                     bool wasSaved,                                     bool wasOverwritten,                                     bool wasRenamed,                                     int oldFileSize);
cfvariant *saveUploadedFile(const webstrada::UploadedFile &file,                                    const std::string &serverDir,                                    const webstrada::string &conflict,                                    const std::string &initialServerFile = std::string());
void resolveUploadDestination(const webstrada::string &destStr, const char *funcName,                                      std::string &serverDir, bool &dirMode,                                      std::string &fileBase);
const cfml::LocaleInfo *currentLocale();
const char *currentLocaleStr();
const cfml::LocaleInfo *resolveLocale(const cfvariant *localeArg);
std::string groupDigits(const std::string &digits, const char *groupSep);
bool parseNumberWithLocale(const string &s, const cfml::LocaleInfo *loc, double &out);
long long roundHalfEven(double v);
std::string formatCurrencyPattern(double absNum, const char *pattern, const cfml::LocaleInfo *loc);
std::string stripCurrencySymbol(const std::string &in, const cfml::LocaleInfo *loc);
std::string formatCurrency(double num, const char *type, const cfml::LocaleInfo *loc);
double lsNumberValue(const cfvariant *num, const char *func);
string formatShortestDouble(double value);
std::string cfJsonDouble(double d);
unsigned int javaStringHash(const char *s);
int javaHashMapBucket(const char *key, int capacity);
int javaHashMapCapacity(size_t size);
struct tm daysToTm(double days);
json_object *serialize_json_value(const cfvariant &val, const string &queryFormat, std::set<const void*> &visited);
// `literalBooleans` makes JSON true/false deserialize to literal booleans
// (stringify as true/false like CF's ObjectLoad, which restores a Java
// Boolean), whereas plain deserializeJson() stringifies them as YES/NO.
cfvariant deserialize_json_value(json_object *obj, bool strictMapping, bool literalBooleans = false);
std::string toStdString(const cfvariant *v);
std::string toLower(std::string s);
double toDouble(const cfvariant *v);
int toInt(const cfvariant *v);
bool toBool(const cfvariant *v);

// ---- CFML type-validation checks (shared by isvalid() and <cfparam>) ----
// Pure checks (no throw) mirroring the CF 2025 validator classes in
// coldfusion/tagext/validation; the callers translate a false result into the
// appropriate CF exception. Moved here from fn_isvalid.cpp so <cfparam> reuses
// the exact same logic.
bool cfmlRegexFullMatch(const std::string &pattern, const std::string &subject);
bool cfmlIsUsDate(const std::string &str);
bool cfmlIsEuroDate(const std::string &str);
bool cfmlIsTimeString(const std::string &str);
bool cfmlLuhnCheck(const std::string &digits);
bool cfmlIsValidVariableName(const std::string &id);
bool cfmlIsValidNumeric(const cfvariant *value);
bool cfmlIsValidInteger(const cfvariant *value);
bool cfmlIsValidUrl(const std::string &urlString);
// Strict Java Double.parseDouble / Integer.parseInt-like parsers backing the
// checks above (also used directly by isvalid()'s numeric_legacy path).
bool cfmlStrictParseDouble(const std::string &s, double &out);
bool cfmlStrictParseInt(const std::string &s, long long &out);
bool randAlgorithmValid(const std::string &algo);
void randAlgorithmValidate(const std::string &algo);
const char *randDefaultAlgorithm();
void imageThrow(const char *type, const std::string &message, const std::string &detail);
std::vector<std::byte> readFileBytes(const std::string &path);
std::string resolveSourcePath(const std::string &path);
void writeFileBytes(const std::string &path, const std::vector<std::byte> &data, bool overwrite);
std::string fileExt(const std::string &path);
ImageData *imageAlloc(int w, int h);
void surfaceRGBA(cairo_surface_t *sf, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
void surfaceSetRGBA(cairo_surface_t *sf, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
std::string sniffFormat(const std::vector<std::byte> &bytes);
uint32_t crc32Buffer(const uint8_t *data, size_t len);
void pngChunk2(std::vector<uint8_t> &out, const char type[4], const uint8_t *data, size_t len);
std::vector<std::byte> encodePng(ImageData *img);
ImageData *decodePng(const std::vector<std::byte> &bytes, const std::string &source);
std::vector<std::byte> encodeJpeg(ImageData *img, double quality);
ImageData *decodeJpeg(const std::vector<std::byte> &bytes, const std::string &source);
std::vector<uint8_t> gifLzwDecode(const std::vector<uint8_t> &data, int minCodeSize);
void gifBuildPalette(ImageData *img, std::vector<uint8_t> &palette, std::vector<uint8_t> &indices);
void gifLzwEncode(const std::vector<uint8_t> &pixels, int minCodeSize, std::vector<uint8_t> &out);
std::vector<std::byte> encodeGif(ImageData *img);
ImageData *decodeGif(const std::vector<std::byte> &bytes, const std::string &source);
std::vector<std::byte> encodeBmp(ImageData *img);
ImageData *decodeBmp(const std::vector<std::byte> &bytes, const std::string &source);
std::vector<std::byte> encodePnm(ImageData *img);
ImageData *decodePnm(const std::vector<std::byte> &bytes, const std::string &source);
ImageData *imageFromBytes(const std::vector<std::byte> &bytes, const std::string &formatHint, const std::string &source);
std::vector<std::byte> encodeImage(ImageData *img, const std::string &format, double quality);
bool cfColorName(const std::string &lower, uint32_t &out);
void colorError(const std::string &message);
bool parseIntStrict(const std::string &s, int &out);
uint32_t parseDrawColor(const std::string &raw);
void setupSource(cairo_t *cr, ImageData *img);
void setupStroke(cairo_t *cr, ImageData *img);
void paintShape(ImageData *img, const std::function<void(cairo_t*)> &draw);
void fillRectColor(ImageData *img, int x, int y, int w, int h, uint32_t rgb);
void drawingPostMul(ImageData *img, const cairo_matrix_t &t);
cfvariant *nullResult();
const cfvariant *structGet(const cfvariant *v, const char *key);
cfvariant *imageResult(ImageData *img);
ImageData *imageClone(ImageData *src);
void imageReplaceSurface(ImageData *img, cairo_surface_t *newSurface, int w, int h);
void imgPixel(const ImageData *img, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
void imgSetPixel(ImageData *dst, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void imageTransform(ImageData *img,                            const std::function<void(uint8_t&,uint8_t&,uint8_t&,uint8_t&)> &fn);
uint32_t imgParseColor(const cfvariant *color);
void samplePixel(const ImageData *img, double fx, double fy, bool bilinear,                         uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
std::string vToString(const cfvariant *v);
std::string vToLower(std::string s);
void jpegAppSegments(const std::vector<std::byte> &bytes,                             std::vector<std::vector<std::byte>> &app1,                             std::vector<std::vector<std::byte>> &app13);
int tiffTypeSize(int type);
std::string tiffAscii(const TiffBytes &r, size_t off, int count);
std::string rationalToString(long long num, long long den);
bool rationalIsInteger(long long num, long long den);
bool rationalTooComplex(long long num, long long den);
void rationalSimplified(long long num, long long den, long long &snum, long long &sden);
std::string javaDoubleToString(double d);
std::string rationalToSimpleString(long long num, long long den, bool allowDecimal);
const TiffValue *tiffGet(const TiffDir &dir, int tag);
long long rationalIntValue(long long num, long long den);
std::string tiffGetString(const TiffDir &dir, int tag);
bool javaParseInt(const std::string &s, long long &out);
long long tiffGetInteger(const TiffDir &dir, int tag);
bool tiffGetRational(const TiffDir &dir, int tag, long long &num, long long &den);
bool tiffGetRationalArray(const TiffDir &dir, int tag, std::vector<long long> &num, std::vector<long long> &den);
std::vector<long long> tiffGetIntArray(const TiffDir &dir, int tag);
std::vector<unsigned char> tiffGetByteArray(const TiffDir &dir, int tag);
double tiffGetDouble(const TiffDir &dir, int tag);
float tiffGetFloat(const TiffDir &dir, int tag);
const std::map<int, std::string> &exifTagNames();
const std::map<int, std::string> &gpsTagNames();
std::string tagName(const std::map<int, std::string> &map, int tag);
int tiffPointerKind(int tag, int curKind);
bool tiffHasFollower(int curKind);
void tiffSetValue(TiffDir &dir, int tag, int type, int count, const TiffBytes &r, size_t off);
void parseTiffIfd(const TiffBytes &r, size_t base, size_t ifdOff,                          int kind, std::vector<TiffDir> &dirs, std::vector<size_t> &visited);
bool parseTiff(const std::vector<std::byte> &bytes, size_t tiffStart,                       std::vector<TiffDir> &dirs);
std::string decimalFormat00_00(double d);
std::string javaDecimalFormat(int minFrac, int maxFrac, double d);
double apertureToFStop(double d);
std::string indexedDescription(const TiffDir &dir, int tag, int start,                                       const std::vector<std::string> &values);
std::string versionBytesDescription(const TiffDir &dir, int tag, int insertIndex);
std::string tagDescriptorFallback(const TiffDir &dir, int tag, const TiffValue *v);
std::string exifDescription(const TiffDir &dir, int tag);
std::string gpsDescription(const TiffDir &dir, int tag);
const std::map<int, std::string> &iptcTagNames();
std::vector<IptcEntry> parseIptc(const std::vector<std::byte> &payload);
std::string iptcDescription(const IptcEntry &e);
std::vector<std::byte> imageMetaBytes(ImageData *img);
void buildExifStruct(ImageData *img, cfvariant &s);
void buildIptcStruct(ImageData *img, cfvariant &s);
std::string normalizeRePattern(const std::string &pat);
pcre2_code *reCompile(const std::string &pattern, bool nocase);
bool reFindNext(pcre2_code *code, const std::string &subject,                 PCRE2_SIZE startOffset, ReMatchResult &out);
webstrada::cfvariant *makeReFindStruct(const ReMatchResult &m, const std::string &subject);
webstrada::cfvariant *makeReFindEmptyStruct();
webstrada::cfvariant *doReFind(const webstrada::string &reVal, const webstrada::string &strVal,                             int start, bool returnsub, const webstrada::string &scopeVal, bool nocase);
webstrada::cfvariant *doReMatch(const webstrada::string &reVal, const webstrada::string &strVal, bool nocase);
std::string substitutionPass(const std::string &subject, const std::string &pattern,                              char appendLiteral);
std::string preprocessReplacement(const std::string &subst);
std::string renderSubstitution(const std::string &subst, const std::string &subject,                                const ReMatchResult &m);
webstrada::cfvariant *doReReplace(const webstrada::string &strVal, const webstrada::string &reVal,                                const webstrada::string &subVal, const webstrada::string &scopeVal, bool nocase);
std::vector<webstrada::string> getSortedComponentKeys(const webstrada::cfvariant *compVal, bool includeProperties, bool includeThisScope, bool includeMethods);

} // namespace cfml

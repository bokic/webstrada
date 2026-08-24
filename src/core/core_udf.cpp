#include "core_internal.h"
#include "../cftags/common.h"

#include <webstrada/cf8.h>
#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
#include <webstrada/worker.h>
#include <webstrada/cfimage.h>
#include <webstrada/cfvariant.h>
#include <webstrada/string.h>
#include <webstrada/scope_store.h>
#include <webstrada/config.h>
#include <webstrada/locale.h>
#include <webstrada/cfimage.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/provider.h>

#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>

using namespace webstrada;
using namespace cfml;
// ---- Built-in function-name registry + UDF runtime ----


// The set of CFML built-in function names, derived from cfml_docs/
// CFML_FUNCTION_*.md. A bare reference to one of these names (e.g. #pi#,
// #abs#) evaluates to a method handle like ColdFusion's
// coldfusion.runtime.CFPageMethod instead of throwing "Variable X is
// undefined." (previously BUGS.md #3).
static const char *const kBuiltinFunctionNames[] = {
    "ABS", "ACOS", "ADDSOAPREQUESTHEADER", "ADDSOAPRESPONSEHEADER", "AJAXLINK", "AJAXONLOAD", "APPLICATIONSTOP", "ARRAYAPPEND",
    "ARRAYAVG", "ARRAYCLEAR", "ARRAYCONTAINS", "ARRAYCONTAINSNOCASE", "ARRAYDELETE", "ARRAYDELETEAT", "ARRAYDELETENOCASE", "ARRAYEACH",
    "ARRAYFILTER", "ARRAYFIND", "ARRAYFINDALL", "ARRAYFINDALLNOCASE", "ARRAYFINDNOCASE", "ARRAYINSERTAT", "ARRAYISDEFINED", "ARRAYISEMPTY",
    "ARRAYLEN", "ARRAYMAP", "ARRAYMAX", "ARRAYMIN", "ARRAYNEW", "ARRAYPOP", "ARRAYPREPEND", "ARRAYREDUCE", "ARRAYRESIZE",
    "ARRAYSET", "ARRAYSETMETADATA", "ARRAYSHIFT", "ARRAYSLICE", "ARRAYSORT", "ARRAYSUM", "ARRAYSWAP", "ARRAYTOLIST", "ARRAYFIRST", "ARRAYLAST", "ASC",
    "ASIN", "ATN", "AUTHENTICATEDCONTEXT", "AUTHENTICATEDUSER", "BINARYDECODE", "BINARYENCODE", "BITAND", "BITMASKCLEAR",
    "BITMASKREAD", "BITMASKSET", "BITNOT", "BITOR", "BITSHLN", "BITSHRN", "BITXOR", "BOOLEANFORMAT",
    "CACHEGET", "CACHEGETALLIDS", "CACHEGETMETADATA", "CACHEGETPROPERTIES", "CACHEGETSESSION", "CACHEIDEXISTS", "CACHEPUT", "CACHEREGIONEXISTS",
    "CACHEREGIONNEW", "CACHEREGIONREMOVE", "CACHEREMOVE", "CACHEREMOVEALL", "CACHESETPROPERTIES", "CALLSTACKDUMP", "CALLSTACKGET", "CANDESERIALIZE",
    "CANONICALIZE", "CANSERIALIZE", "CEILING", "CHARSETDECODE", "CHARSETENCODE", "CHR", "CJUSTIFY", "COMPARE",
    "COMPARENOCASE", "COS", "CREATEDATE", "CREATEDATETIME", "CREATEENCRYPTEDJWT", "CREATEOBJECT", "CREATEODBCDATE", "CREATEODBCDATETIME",
    "CREATEODBCTIME", "CREATESIGNEDJWT", "CREATETIME", "CREATETIMESPAN", "CREATEUUID", "CSRFGENERATETOKEN", "CSRFVERIFYTOKEN", "CSVPROCESS",
    "CSVREAD", "CSVWRITE", "DATEADD", "DATECOMPARE", "DATECONVERT", "DATEDIFF", "DATEFORMAT", "DATEPART",
    "DATETIMEFORMAT", "DAY", "DAYOFWEEK", "DAYOFWEEKASSTRING", "DAYOFYEAR", "DAYSINMONTH", "DAYSINYEAR", "DE",
    "DECIMALFORMAT", "DECODEFORHTML", "DECODEFROMURL", "DECREMENTVALUE", "DECRYPT", "DECRYPTBINARY", "DELETECLIENTVARIABLE", "DESERIALIZE",
    "DESERIALIZEJSON", "DESERIALIZEXML", "DIRECTORYCOPY", "DIRECTORYCREATE", "DIRECTORYDELETE", "DIRECTORYEXISTS", "DIRECTORYLIST", "DIRECTORYRENAME",
    "DOLLARFORMAT", "DOTNETTOCFTYPE", "DUPLICATE", "ENCODEFORCSS", "ENCODEFORDN", "ENCODEFORHTML", "ENCODEFORHTMLATTRIBUTE", "ENCODEFORJAVASCRIPT",
    "ENCODEFORLDAP", "ENCODEFORURL", "ENCODEFORXML", "ENCODEFORXMLATTRIBUTE", "ENCODEFORXPATH", "ENCRYPT", "ENCRYPTBINARY", "ENTITYDELETE",
    "ENTITYLOAD", "ENTITYLOADBYEXAMPLE", "ENTITYLOADBYPK", "ENTITYMERGE", "ENTITYNEW", "ENTITYRELOAD", "ENTITYSAVE", "ENTITYTOQUERY",
    "EVALUATE", "EXP", "EXPANDPATH", "FILECLOSE", "FILECOPY", "FILEDELETE", "FILEEXISTS", "FILEGETMIMETYPE",
    "FILEISEOF", "FILEMOVE", "FILEOPEN", "FILEREAD", "FILEREADBINARY", "FILEREADLINE", "FILESEEK", "FILESETACCESSMODE",
    "FILESETATTRIBUTE", "FILESETLASTMODIFIED", "FILESKIPBYTES", "FILEUPLOAD", "FILEUPLOADALL", "FILEWRITE", "FILEWRITELINE", "FIND",
    "FINDNOCASE", "FINDONEOF", "FIRSTDAYOFMONTH", "FIX", "FLOOR", "FORMATBASEN", "GENERATE3DESKEY", "GENERATEPBKDFKEY",
    "GENERATESAMLSPMETADATA", "GENERATESECRETKEY", "GETAPPLICATIONMETADATA", "GETAUTHUSER", "GETBASETAGDATA", "GETBASETAGLIST", "GETBASETEMPLATEPATH", "GETCLIENTVARIABLESLIST",
    "GETCOMPONENTMETADATA", "GETCONTEXTROOT", "GETCPUUSAGE", "GETCSPNONCE", "GETCURRENTTEMPLATEPATH", "GETDIRECTORYFROMPATH", "GETENCODING", "GETEXCEPTION",
    "GETFILEFROMPATH", "GETFILEINFO", "GETFREESPACE", "GETGATEWAYHELPER", "GETHTTPREQUESTDATA", "GETHTTPTIMESTRING", "GETK2SERVERDOCCOUNT", "GETK2SERVERDOCCOUNTLIMIT",
    "GETLOCALE", "GETLOCALEDISPLAYNAME", "GETLOCALHOSTIP", "GETMETADATA", "GETMETRICDATA", "GETPAGECONTEXT", "GETPRINTERINFO", "GETPRINTERLIST",
    "GETPROFILESECTIONS", "GETPROFILESTRING", "GETPROPERTYFILE", "GETPROPERTYSTRING", "GETREADABLEIMAGEFORMATS", "GETSAFEHTML", "GETSAMLAUTHREQUEST", "GETSAMLLOGOUTREQUEST",
    "GETSOAPREQUEST", "GETSOAPREQUESTHEADER", "GETSOAPRESPONSE", "GETSOAPRESPONSEHEADER", "GETSYSTEMFREEMEMORY", "GETSYSTEMTOTALMEMORY",     "GETTEMPDIRECTORY", "GETTEMPFILE",
    "GETTICKCOUNT", "GETTIMEZONEINFO", "GETTOKEN", "GETTOTALSPACE", "GETUSERROLES", "GETVFSMETADATA", "GETWRITEABLEIMAGEFORMATS",
    "HASH", "HMAC", "HOUR", "HQLMETHODS", "HTMLCODEFORMAT", "HTMLEDITFORMAT", "IIF", "IMAGEADDBORDER",
    "IMAGEBLUR", "IMAGECLEARRECT", "IMAGECOPY", "IMAGECREATECAPTCHA", "IMAGECROP", "IMAGEDRAWARC", "IMAGEDRAWBEVELEDRECT", "IMAGEDRAWCUBICCURVE",
    "IMAGEDRAWLINE", "IMAGEDRAWLINES", "IMAGEDRAWOVAL", "IMAGEDRAWPOINT", "IMAGEDRAWQUADRATICCURVE", "IMAGEDRAWRECT", "IMAGEDRAWROUNDRECT", "IMAGEDRAWTEXT",
    "IMAGEFLIP", "IMAGEGETBLOB", "IMAGEGETBUFFEREDIMAGE", "IMAGEGETEXIFMETADATA", "IMAGEGETEXIFTAG", "IMAGEGETHEIGHT", "IMAGEGETIPTCMETADATA", "IMAGEGETIPTCTAG",
    "IMAGEGETMETADATA", "IMAGEGETWIDTH", "IMAGEGRAYSCALE", "IMAGEINFO", "IMAGEMAKECOLORTRANSPARENT", "IMAGEMAKETRANSLUCENT", "IMAGENEGATIVE", "IMAGENEW",
    "IMAGEOVERLAY", "IMAGEPASTE", "IMAGEREAD", "IMAGEREADBASE64", "IMAGERESIZE", "IMAGEROTATE", "IMAGEROTATEDRAWINGAXIS", "IMAGESCALETOFIT",
    "IMAGESETANTIALIASING", "IMAGESETBACKGROUNDCOLOR", "IMAGESETDRAWINGCOLOR", "IMAGESETDRAWINGSTROKE", "IMAGESETDRAWINGTRANSPARENCY", "IMAGESHARPEN", "IMAGESHEAR", "IMAGESHEARDRAWINGAXIS",
    "IMAGETRANSLATE", "IMAGETRANSLATEDRAWINGAXIS", "IMAGEWRITE", "IMAGEWRITEBASE64", "IMAGEXORDRAWINGMODE", "INCREMENTVALUE", "INITSAMLAUTHREQUEST", "INITSAMLLOGOUTREQUEST",
    "INPUTBASEN", "INSERT", "INT", "INTERRUPTTHREAD", "INVALIDATEOAUTHACCESSTOKEN", "INVOKE", "ISARRAY", "ISAUTHENTICATED",
    "ISAUTHORIZED", "ISBINARY", "ISBOOLEAN", "ISCLOSURE", "ISDATE", "ISDATEOBJECT", "ISDDX", "ISDEBUGMODE",
    "ISDEFINED", "ISIMAGE", "ISIMAGEFILE", "ISINSTANCEOF", "ISIPV6", "ISJSON", "ISK2SERVERABROKER", "ISK2SERVERDOCCOUNTEXCEEDED",
    "ISK2SERVERONLINE", "ISLEAPYEAR", "ISLOCALHOST", "ISNULL", "ISNUMERIC", "ISNUMERICDATE", "ISOBJECT", "ISONLINE",
    "ISPDFARCHIVE", "ISPDFFILE", "ISPDFOBJECT", "ISPROTECTED", "ISQUERY", "ISSAFEHTML", "ISSAMLLOGOUTRESPONSE", "ISSIMPLEVALUE",
    "ISSOAPREQUEST", "ISSPREADSHEETFILE", "ISSPREADSHEETOBJECT", "ISSTRUCT", "ISTHREADINTERRUPTED", "ISUSERINANYROLE", "ISUSERINROLE", "ISUSERLOGGEDIN",
    "ISVALID", "ISVALIDOAUTHACCESSTOKEN", "ISWDDX", "ISXML", "ISXMLATTRIBUTE", "ISXMLDOC", "ISXMLELEM", "ISXMLNODE",
    "ISXMLROOT", "JAVACAST", "JSSTRINGFORMAT", "LCASE", "LEFT", "LEN", "LISTAPPEND", "LISTCHANGEDELIMS",
    "LISTCONTAINS", "LISTCONTAINSNOCASE", "LISTDELETEAT", "LISTEACH", "LISTFILTER", "LISTFIND", "LISTFINDNOCASE", "LISTFIRST",
    "LISTGETAT", "LISTGETDUPLICATES", "LISTINSERTAT", "LISTLAST", "LISTLEN", "LISTMAP", "LISTPREPEND", "LISTQUALIFY",
    "LISTREDUCE", "LISTREMOVEDUPLICATES", "LISTREST", "LISTSETAT", "LISTSORT", "LISTTOARRAY", "LISTVALUECOUNT", "LISTVALUECOUNTNOCASE",
    "LJUSTIFY", "LOCATION", "LOG", "LOG10", "LSCURRENCYFORMAT", "LSDATEFORMAT", "LSDATETIMEFORMAT", "LSEUROCURRENCYFORMAT",
    "LSISCURRENCY", "LSISDATE", "LSISNUMERIC", "LSNUMBERFORMAT", "LSPARSECURRENCY", "LSPARSEDATETIME", "LSPARSEEUROCURRENCY", "LSPARSENUMBER",
    "LSTIMEFORMAT", "LTRIM", "MAX", "MID", "MIN", "MINUTE", "MONTH", "MONTHASSTRING",
    "NOW", "NUMBERFORMAT", "OBJECTEQUALS", "OBJECTLOAD", "OBJECTSAVE", "ONWSAUTHENTICATE", "ORMCLEARSESSION", "ORMCLOSEALLSESSIONS",
    "ORMCLOSESESSION", "ORMEVICTCOLLECTION", "ORMEVICTENTITY", "ORMEVICTQUERIES", "ORMEXECUTEQUERY", "ORMFLUSH", "ORMFLUSHALL", "ORMGETSESSION",
    "ORMGETSESSIONFACTORY", "ORMINDEX", "ORMINDEXPURGE", "ORMRELOAD", "ORMSEARCH", "ORMSEARCHOFFLINE", "PARAGRAPHFORMAT",
    "PARSEDATETIME", "PI", "PRECISIONEVALUATE", "PRESERVESINGLEQUOTES", "PROCESSSAMLLOGOUTREQUEST", "PROCESSSAMLRESPONSE", "QUARTER", "QUERYADDCOLUMN",
    "QUERYADDROW", "QUERYCONVERTFORGRID", "QUERYEACH", "QUERYEXECUTE", "QUERYFILTER", "QUERYGETRESULT", "QUERYGETROW", "QUERYKEYEXISTS",
    "QUERYMAP", "QUERYNEW", "QUERYREDUCE", "QUERYSETCELL", "QUOTEDVALUELIST", "RAND", "RANDOMIZE", "RANDRANGE",
    "REESCAPE", "REFIND", "REFINDNOCASE", "RELEASECOMOBJECT", "REMATCH", "REMATCHNOCASE", "REMOVECACHEDQUERY", "REMOVECHARS",
    "REPEATSTRING", "REPLACE", "REPLACELIST", "REPLACENOCASE", "REREPLACE", "REREPLACENOCASE", "RESTDELETEAPPLICATION", "RESTINITAPPLICATION",
    "RESTSETRESPONSE", "REVERSE", "RIGHT", "RJUSTIFY", "ROUND", "RTRIM", "SECOND", "SENDGATEWAYMESSAGE",
    "SENDSAMLLOGOUTRESPONSE", "SERIALIZE", "SERIALIZEJSON", "SERIALIZEXML", "SESSIONGETMETADATA", "SESSIONINVALIDATE", "SESSIONROTATE", "SETDAY",
    "SETENCODING", "SETHOUR", "SETLOCALE", "SETMONTH", "SETPROFILESTRING", "SETPROPERTYSTRING", "SETVARIABLE", "SETYEAR",
    "SGN", "SIN", "SLEEP", "SPANEXCLUDING", "SPANINCLUDING", "SPREADSHEETADDAUTOFILTER", "SPREADSHEETADDCOLUMN", "SPREADSHEETADDFREEZEPANE",
    "SPREADSHEETADDIMAGE", "SPREADSHEETADDINFO", "SPREADSHEETADDPAGEBREAKS", "SPREADSHEETADDPRINTGRIDLINES", "SPREADSHEETADDROW", "SPREADSHEETADDROWS", "SPREADSHEETADDSPLITPANE", "SPREADSHEETCREATESHEET",
    "SPREADSHEETDELETECOLUMN", "SPREADSHEETDELETECOLUMNS", "SPREADSHEETDELETEROW", "SPREADSHEETDELETEROWS", "SPREADSHEETFORMATCELL", "SPREADSHEETFORMATCELLRANGE", "SPREADSHEETFORMATCOLUMN", "SPREADSHEETFORMATCOLUMNS",
    "SPREADSHEETFORMATROW", "SPREADSHEETFORMATROWS", "SPREADSHEETGETCELLCOMMENT", "SPREADSHEETGETCELLFORMULA", "SPREADSHEETGETCELLVALUE", "SPREADSHEETGETCOLUMNCOUNT", "SPREADSHEETGETCOLUMNWIDTH", "SPREADSHEETGETLASTROWNUMBER",
    "SPREADSHEETGETPRINTORIENTATION", "SPREADSHEETGROUPCOLUMNS", "SPREADSHEETGROUPROWS", "SPREADSHEETINFO", "SPREADSHEETISBINARYFORMAT", "SPREADSHEETISCOLUMNHIDDEN", "SPREADSHEETISROWHIDDEN", "SPREADSHEETISSTREAMINGXMLFORMAT",
    "SPREADSHEETISXMLFORMAT", "SPREADSHEETMERGECELLS", "SPREADSHEETNEW", "SPREADSHEETREAD", "SPREADSHEETREADBINARY", "SPREADSHEETREMOVECOLUMNBREAK", "SPREADSHEETREMOVEPRINTGRIDLINES", "SPREADSHEETREMOVEROWBREAK",
    "SPREADSHEETREMOVESHEET", "SPREADSHEETREMOVESHEETNUMBER", "SPREADSHEETRENAMESHEET", "SPREADSHEETSETACTIVESHEET", "SPREADSHEETSETACTIVESHEETNUMBER", "SPREADSHEETSETCELLCOMMENT", "SPREADSHEETSETCELLFORMULA", "SPREADSHEETSETCELLVALUE",
    "SPREADSHEETSETCOLUMNBREAK", "SPREADSHEETSETCOLUMNHIDDEN", "SPREADSHEETSETCOLUMNWIDTH", "SPREADSHEETSETFITTOPAGE", "SPREADSHEETSETFOOTER", "SPREADSHEETSETFOOTERIMAGE", "SPREADSHEETSETHEADER", "SPREADSHEETSETHEADERIMAGE",
    "SPREADSHEETSETROWBREAK", "SPREADSHEETSETROWHEIGHT", "SPREADSHEETSETROWHIDDEN", "SPREADSHEETSHIFTCOLUMNS", "SPREADSHEETSHIFTROWS", "SPREADSHEETUNGROUPCOLUMNS", "SPREADSHEETUNGROUPROWS", "SPREADSHEETWRITE",
    "SQR", "STOREADDACL", "STOREGETACL", "STOREGETMETADATA", "STORESETACL", "STORESETMETADATA", "STREAMINGSPREADSHEETCLEANUP", "STREAMINGSPREADSHEETISSTREAMINGXMLFORMAT",
    "STREAMINGSPREADSHEETISXMLFORMAT", "STREAMINGSPREADSHEETNEW", "STREAMINGSPREADSHEETPROCESS", "STREAMINGSPREADSHEETREAD", "STRIPCR", "STRUCTAPPEND", "STRUCTCLEAR", "STRUCTCOPY", "STRUCTCOUNT", "STRUCTDELETE",
    "STRUCTEACH", "STRUCTFILTER", "STRUCTFIND", "STRUCTFINDKEY", "STRUCTFINDVALUE", "STRUCTGET", "STRUCTGETMETADATA", "STRUCTINSERT",
    "STRUCTISEMPTY", "STRUCTKEYARRAY", "STRUCTKEYEXISTS", "STRUCTKEYLIST", "STRUCTMAP", "STRUCTNEW", "STRUCTREDUCE", "STRUCTSETMETADATA",
    "STRUCTSORT", "STRUCTTOSORTED", "STRUCTUPDATE", "STRUCTVALUEARRAY", "TAN", "THREADJOIN", "THREADTERMINATE", "THROW",
    "TIMEFORMAT", "TOBASE64", "TOBINARY", "TOSCRIPT", "TOSTRING", "TRACE", "TRANSACTIONCOMMIT", "TRANSACTIONROLLBACK",
    "TRANSACTIONSETSAVEPOINT", "TRIM", "UCASE", "URLDECODE", "URLENCODEDFORMAT", "URLSESSIONFORMAT", "VAL", "VALUELIST",
    "VERIFYCLIENT", "WEEK", "WRAP", "WRITEDUMP", "WRITELOG", "WRITEOUTPUT", "WSGETALLCHANNELS", "WSGETSUBSCRIBERS",
    "WSPUBLISH", "WSSENDMESSAGE", "XMLCHILDPOS", "XMLELEMNEW", "XMLFORMAT", "XMLGETNODETYPE", "XMLNEW", "XMLPARSE",
    "XMLSEARCH", "XMLTRANSFORM", "XMLVALIDATE", "YEAR", "YESNOFORMAT",
};

// True when the whole expression is a single bare identifier (no member access,
// array index, call, quoting or whitespace), i.e. exactly the shape that can be
// a variable or a bare built-in function reference.
bool isBareIdentifier(const string &s)
{
    string t = s.trimmed();
    if (t.isEmpty()) return false;
    char c0 = t.at(0);
    if (!(isalpha(static_cast<unsigned char>(c0)) || c0 == '_')) return false;
    for (size_t i = 0; i < t.length(); i++) {
        char c = t.at(i);
        if (!(isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
    }
    return true;
}

bool isKnownFunctionName(const string &name)
{
    string u = name;
    u.toUpper();
    for (const char *fn : kBuiltinFunctionNames) {
        if (u.equals(fn)) return true;
    }
    return false;
}

std::vector<std::string> cfml::builtinFunctionNames()
{
    std::vector<std::string> out;
    for (const char *fn : kBuiltinFunctionNames) {
        if (fn) out.emplace_back(fn);
    }
    return out;
}

// ColdFusion renders a bare built-in function reference as a method handle
// (coldfusion.runtime.CFPageMethod@<hash>). The hash is the JVM identity hash
// of the handle object, which differs per server run; here it is a stable Java
// String.hashCode() of the function name so the output is deterministic.
string functionHandleText(const string &name)
{
    unsigned int h = 0;
    for (size_t i = 0; i < name.length(); i++) {
        char c = name.at(i);
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        h = h * 31u + (unsigned char)c;
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%x", h);
    return string("coldfusion.runtime.CFPageMethod@") + buf;
}

cfvariant makeFunctionHandle(const string &name)
{
    cfvariant fn(cfvariant::Function);
    *fn.m_str = functionHandleText(name);
    return fn;
}

// ---- User-defined functions / closures runtime ----

// Innermost-first stack of active UDF invocations. lookupVarWritable and
// cfvariant_assign consult it so a UDF body's unqualified reads fall back to
// the captured parent scope and its unqualified writes go to the parent scope
// unless the name is a local (param / var / nested function / arguments).
thread_local std::vector<UdfCallCtx> g_udfCtx;

// ColdFusion's `searchimplicitscopes` toggle: when false (the default) an
// unqualified name is only searched in the variables scope (and function-local
// scopes), never in the implicit scopes (CGI, FILE, URL, FORM, COOKIE, CLIENT).
// SERVER / APPLICATION / SESSION are NEVER searched for unqualified names
// regardless of this flag. <cfapplication searchimplicitscopes="yes"> enables it.
// Defined in cftags/tag_application.cpp (declared in cftags/common.h).

void cfml::cf_udf_begin(cfvariant *localScope, cfvariant *parentScope)
{
    UdfCallCtx ctx;
    ctx.localScope = localScope;
    ctx.parentScope = parentScope;
    g_udfCtx.push_back(std::move(ctx));
}

webstrada::cfvariant *udfVariablesScope(webstrada::cfvariant *passedVariables)
{
    // Inside a plain (non-component) UDF, `variables` (the body's passed
    // argument, which aliases the function local scope) is CF's CALLING page's
    // variables scope — the captured parent scope (was BUGS.md "UDF:
    // variables.foo"). In a component method, `variables` is the instance's
    // variables scope (which is what was passed as passedVariables).
    for (auto it = g_udfCtx.rbegin(); it != g_udfCtx.rend(); ++it) {
        if (it->parentScope && passedVariables == it->localScope && !it->component) {
            return it->parentScope;
        }
    }
    return passedVariables;
}

void cfml::cf_udf_mark_local(const char *name)
{
    if (g_udfCtx.empty() || !name) return;
    webstrada::string n(name);
    n.toUpper();
    g_udfCtx.back().localNames.insert(n);
}

webstrada::cfvariant *udfArgumentsScope(const webstrada::cfvariant *localScope)
{
    if (!localScope || localScope->m_type != cfvariant::Struct || localScope->m_disabled) return nullptr;
    auto it = localScope->m_struct->find("ARGUMENTS");
    if (it == localScope->m_struct->end() || it->second.m_type != cfvariant::Struct) return nullptr;
    return &it->second;
}

webstrada::cfvariant *udfAssignScope(webstrada::cfvariant *variables, const char *name)
{
    if (g_udfCtx.empty() || !name) return variables;
    webstrada::string uname(name);
    uname.toUpper();
    if (g_udfCtx.back().localNames.find(uname) != g_udfCtx.back().localNames.end()) {
        return g_udfCtx.back().localScope;
    }
    cfvariant *args = udfArgumentsScope(g_udfCtx.back().localScope);
    if (args) {
        auto it = args->m_struct->find(uname);
        if (it != args->m_struct->end() && it->second.m_type != cfvariant::Null) {
            return args;
        }
    }
    return g_udfCtx.back().parentScope;
}
void cfml::cf_udf_remove_params(cfvariant *localScope, const char **paramNames, int paramCount)
{
    if (!localScope || localScope->m_type != cfvariant::Struct || localScope->m_disabled) return;
    for (int i = 0; i < paramCount; i++) {
        if (!paramNames[i]) continue;
        webstrada::string key(paramNames[i]);
        key.toUpper();
        struct_data_bump(localScope->m_structData);
        localScope->m_struct->erase(key);
    }
}

void cfml::cf_udf_end()
{
    if (!g_udfCtx.empty()) g_udfCtx.pop_back();
}

cfvariant *cfml::cfvariant_create_udf(const char *name, void *fn, bool isClosure, cfvariant *capturedScope, const void *metaBlob)
{
    auto *info = new UDFInfo();
    info->fn = fn;
    if (name) info->name = name;
    info->isClosure = isClosure;
    info->capturedScope = capturedScope;
    info->access = "public";
    if (metaBlob) {
        UdfMetaInfo meta;
        udf_meta_deserialize(static_cast<const char*>(metaBlob), meta);
        info->params = std::move(meta.params);
        info->returnType = meta.returnType;
        if (!meta.access.isEmpty()) info->access = meta.access;
    }
    auto *ret = new cfvariant(cfvariant::Function);
    *ret->m_str = info->name;
    ret->m_udf = info;
    // The JIT's emitCall whitelist explicitly excludes cfvariant_create_udf (the
    // value is stored into the variables scope), so register the temp here; the
    // request cleanup frees it and the scope holds a refcounted copy.
    cf_register_temp(ret);
    return ret;
}

void cfml::cf_udf_register_temp(cfvariant *val)
{
}

bool cfml::cf_is_known_function_name(const char *name)
{
    if (!name) return false;
    return isKnownFunctionName(name);
}

// Tries to parse a string as a number; returns false when the whole string is
// not numeric (CF rejects such values for typed params/returns).
static bool tryParseNumeric(const webstrada::string &s, double &out)
{
    const char *p = s.constData();
    if (!p || *p == '\0') return false;
    char *end = nullptr;
    out = strtod(p, &end);
    return end != p && *end == '\0';
}

// Converts a value to a CF numeric type (Number when integral and within
// int32, else Float). Throws when not convertible.
static cfvariant *coerceToNumber(const cfvariant *val, const char *argName, const char *funcName, const char *typeName, bool isReturn)
{
    switch (val->m_type) {
        case cfvariant::Number:
        case cfvariant::Long:
        case cfvariant::Float:
        case cfvariant::DateTime: {
            auto *ret = new cfvariant(*val);
            return ret;
        }
        case cfvariant::Boolean: {
            auto *ret = new cfvariant(cfvariant::Number);
            ret->m_int = val->m_bool ? 1 : 0;
            return ret;
        }
        default: {
            double d = 0;
            if (val->m_type == cfvariant::String && val->m_str && tryParseNumeric(*val->m_str, d)) {
                if (d == static_cast<long long>(d) && d >= -2147483648.0 && d <= 2147483647.0) {
                    auto *ret = new cfvariant(static_cast<int>(d));
                    return ret;
                }
                auto *ret = new cfvariant(cfvariant::Float);
                ret->m_double = d;
                return ret;
            }
        }
    }
    webstrada::string msg = isReturn
        ? webstrada::string("The value returned from the ") + funcName + " function is not of type " + typeName + "."
        : webstrada::string("The ") + argName + " argument passed to the " + funcName + " function is not of type " + typeName + ".";
    throw webstrada::exception(msg);
}

// Throws the CF MissingArgumentException message for a required <cfargument>
// that was not passed in (verified on CF: "The X parameter to the Y function
// is required but was not passed in.").
void cfml::cf_throw_missing_argument(const char *paramName, const char *funcName)
{
    std::string msg = "The ";
    msg += (paramName ? paramName : "");
    msg += " parameter to the ";
    msg += (funcName ? funcName : "");
    msg += " function is required but was not passed in.";
    throw webstrada::exception(msg.c_str());
}

cfvariant *cfml::cf_udf_coerce_arg(const cfvariant *val, const char *typeName, const char *argName, const char *funcName)
{
    // cf_udf_coerce_arg is in the JIT emitCall exclusion list (cf_udf_*), so
    // the fresh result must be registered here to be freed by the request
    // cleanup (idempotent against any caller-side registration).
    if (!val) throw webstrada::exception("Function argument cannot be null");
    if (!typeName || typeName[0] == '\0') {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    webstrada::string t(typeName);
    t.toLower();
    if (t.equals("any")) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("numeric") || t.equals("number") || t.equals("integer") || t.equals("int") ||
        t.equals("long") || t.equals("float") || t.equals("double")) {
        cfvariant *ret = coerceToNumber(val, argName, funcName, typeName, false);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("string")) {
        auto *ret = new cfvariant(const_cast<cfvariant*>(val)->toString());
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("boolean")) {
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = isTruthy(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("array")) {
        if (val->m_type == cfvariant::Array) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    } else if (t.equals("xml")) {
        if (val->m_type == cfvariant::Xml) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    } else if (t.equals("struct")) {
        if (val->m_type == cfvariant::Struct || val->m_type == cfvariant::Xml) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    } else if (t.equals("date")) {
        if (val->m_type == cfvariant::DateTime) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    } else if (t.equals("query")) {
        if (val->m_type == cfvariant::Query) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    } else if (t.equals("component") || t.equals("object")) {
        if (val->m_type == cfvariant::Component) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    } else if (t.equals("binary")) {
        if (val->m_type == cfvariant::Binary) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    } else if (t.equals("uuid") || t.equals("guid")) {
        // CF validates the format; accept any string value here.
        auto *ret = new cfvariant(const_cast<cfvariant*>(val)->toString());
        cf_register_temp(ret);
        return ret;
    } else if (val->m_type == cfvariant::Component) {
        cfvariant typeVar(typeName);
        cfvariant *instMatch = cf_isinstanceof_impl(val, &typeVar);
        bool match = instMatch && instMatch->m_bool;
        delete instMatch;
        if (match) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    }
    // Unknown type names and mismatches throw the CF InvalidArgumentTypeException message.
    throw webstrada::exception(webstrada::string("The ") + argName + " argument passed to the " + funcName +
                              " function is not of type " + typeName + ".");
}

cfvariant *cfml::cf_udf_coerce_return(cfvariant *val, const char *returnType, const char *funcName)
{
    // Always return a fresh, registered temporary so the UDF return value is
    // owned exactly once by the request cleanup. cf_udf_coerce_return is in the
    // JIT emitCall exclusion list (it is only ever called as part of a UDF's
    // return expression), so the registration must happen here.
    if (!returnType || returnType[0] == '\0') {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    webstrada::string t(returnType);
    t.toLower();
    if (t.equals("void")) {
        throw webstrada::exception(webstrada::string("The function ") + funcName + " defined as void tried to return a value.");
    }
    if (t.equals("any")) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (!val) {
        auto *ret = new cfvariant(cfvariant::Null);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("numeric") || t.equals("number") || t.equals("integer") || t.equals("int") ||
        t.equals("long") || t.equals("float") || t.equals("double")) {
        cfvariant *ret = coerceToNumber(val, "", funcName, returnType, true);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("string")) {
        auto *ret = new cfvariant(val->toString());
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("boolean")) {
        auto *ret = new cfvariant(cfvariant::Boolean);
        ret->m_bool = isTruthy(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("array") && val->m_type == cfvariant::Array) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if ((t.equals("struct") || t.equals("xml")) && (val->m_type == cfvariant::Struct || val->m_type == cfvariant::Xml)) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("component") && val->m_type == cfvariant::Component) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("date") && val->m_type == cfvariant::DateTime) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("query") && val->m_type == cfvariant::Query) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (t.equals("binary") && val->m_type == cfvariant::Binary) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if ((t.equals("uuid") || t.equals("guid")) && val->m_type == cfvariant::String) {
        auto *ret = new cfvariant(*val);
        cf_register_temp(ret);
        return ret;
    }
    if (val->m_type == cfvariant::Component) {
        cfvariant typeVar(returnType);
        cfvariant *instMatch = cf_isinstanceof_impl(val, &typeVar);
        bool match = instMatch && instMatch->m_bool;
        delete instMatch;
        if (match) {
            auto *ret = new cfvariant(*val);
            cf_register_temp(ret);
            return ret;
        }
    }
    throw webstrada::exception(webstrada::string("The value returned from the ") + funcName + " function is not of type " + returnType + ".");
}

void cfml::cf_udf_args_metadata(cfvariant *arguments, int paramCount)
{
    if (!arguments || arguments->m_type != cfvariant::Struct) return;
    arguments->m_isArguments = true;
    arguments->m_argumentsParamCount = paramCount;
}

void cfml::cf_udf_args_set_or_null(cfvariant *arguments, const char *key, cfvariant *val)
{
    if (!arguments) return;
    if (val) arguments->structSet(key, *val);
    else arguments->structSet(key, cfvariant(cfvariant::Null));
}

void cfml::cf_udf_build_arguments(cfvariant *localScope, const char **paramNames, int paramCount,
                                  const cfvariant **args, int argc)
{
    if (!localScope || localScope->m_type != cfvariant::Struct) return;
    cfvariant *arguments = new cfvariant(cfvariant::Struct);
    arguments->m_isArguments = true;
    arguments->m_argumentsParamCount = paramCount;
    cf_register_temp(arguments);
    for (int i = 0; i < paramCount; i++) {
        webstrada::string key(paramNames[i] ? paramNames[i] : "");
        key.toUpper();
        cfvariant *slot = nullptr;
        auto it = localScope->m_struct->find(key);
        if (it != localScope->m_struct->end()) slot = &it->second;
        cf_udf_args_set_or_null(arguments, key.constData(), slot);
    }
    for (int i = 0; i < argc; i++) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d", i + 1);
        if (args[i]) arguments->structSet(buf, *args[i]);
    }
    localScope->set("ARGUMENTS") = *arguments;
}

int cfml::cfvariant_type(const cfvariant *v)
{
    return v ? static_cast<int>(v->m_type) : 0;
}

cfvariant *cfml::cf_named_args_marker(cfvariant *namedStruct)
{
    auto *marker = new cfvariant(cfvariant::Struct);
    marker->structSet(CFML_NAMED_ARGS_KEY, *namedStruct);
    return marker;
}

bool cfml::cf_named_args_reorder(const cfvariant **args, int argc,
                                 const char **paramNames, int paramCount,
                                 std::vector<const cfvariant*> &out, int &outArgc)
{
    out.clear();
    outArgc = argc;
    if (argc <= 0 || !args || !args[0]) return false;
    const cfvariant *marker = args[0];
    if (marker->m_type != cfvariant::Struct || !marker->m_struct) return false;
    auto it = marker->m_struct->find(CFML_NAMED_ARGS_KEY);
    if (it == marker->m_struct->end()) return false;
    const cfvariant &namedStruct = it->second;
    if (namedStruct.m_type != cfvariant::Struct || !namedStruct.m_struct) return false;

    // Map each declared parameter name to a slot.
    std::vector<const cfvariant*> slots(paramCount > 0 ? paramCount : 0, nullptr);
    // Place the named arguments into their matching parameter slots.
    for (const auto &kv : *namedStruct.m_struct) {
        const webstrada::string &k = kv.first;
        bool matched = false;
        for (int p = 0; p < paramCount; p++) {
            webstrada::string pn(paramNames[p] ? paramNames[p] : "");
            if (pn.compareCaseInsensitive(k) == 0) {
                slots[p] = &kv.second;
                matched = true;
                break;
            }
        }
        if (!matched) {
            webstrada::string msg("The argument name ");
            msg.append(k);
            msg.append(" was not found in the function arguments.");
            throw webstrada::exception(msg);
        }
    }
    // Fill the unfilled slots in order with the positional arguments (args[1..]).
    int posIdx = 1;
    for (int p = 0; p < paramCount; p++) {
        if (slots[p]) continue;
        if (posIdx < argc) {
            slots[p] = args[posIdx];
            posIdx++;
        } else {
            // A missing parameter (no named arg and no positional value): use a
            // NotSet sentinel so the JIT prologue's `argc > i` check sees a
            // present-but-unset slot and falls back to the default. The prologue
            // treats a NotSet arg as "not passed" (see param binding).
            static cfvariant s_notSet(cfvariant::NotSet);
            slots[p] = &s_notSet;
        }
    }
    // Trailing positional args beyond the declared params are kept as-is.
    int trailing = argc - posIdx;
    out.reserve(slots.size() + trailing);
    for (const cfvariant *v : slots) out.push_back(v);
    for (int t = 0; t < trailing; t++) out.push_back(args[posIdx + t]);
    outArgc = static_cast<int>(out.size());
    return true;
}

cfvariant *cfml::cf_udf_invoke(cfvariant *udfVal, const cfvariant **args, int argc,
                               string &out, void *cgi, void *server, void *cookie, void *application,
                               void *session, void *url, void *form, void *variables)
{
    if (!udfVal || udfVal->m_type != cfvariant::Function || !udfVal->m_udf) {
        throw webstrada::exception("Entity has incorrect type for being called as a function.");
    }
    if (udfVal->m_udf->componentMethodIndex >= 0) {
        return cf_component_method_handle_invoke(udfVal, args, argc, out, cgi, server, cookie,
                                                 application, session, url, form);
    }
    if (!udfVal->m_udf->fn) {
        throw webstrada::exception("Entity has incorrect type for being called as a function.");
    }
    UDFInfo *info = udfVal->m_udf;
    // Reorder named arguments (args[0] is the named-args marker) against the
    // declared parameter names so the JIT prologue binds positionally.
    std::vector<const cfvariant*> reordered;
    int effectiveArgc = argc;
    const cfvariant **effectiveArgs = args;
    if (info->params.empty()) {
        std::vector<const char*> emptyNames;
        if (cf_named_args_reorder(args, argc, nullptr, 0, reordered, effectiveArgc)) {
            effectiveArgs = reordered.data();
        }
    } else {
        std::vector<const char*> names;
        for (const auto &p : info->params) names.push_back(p.name.constData());
        if (cf_named_args_reorder(args, argc, names.data(), static_cast<int>(names.size()), reordered, effectiveArgc)) {
            effectiveArgs = reordered.data();
        }
    }
    // The UDF body's prologue creates and registers the local scope and pushes
    // the call context; truncate the context stack afterwards (also on throw).
    size_t ctxSave = g_udfCtx.size();
    udf_entry_fn entry = reinterpret_cast<udf_entry_fn>(info->fn);
    cfvariant *ret = nullptr;
    try {
        ret = entry(&out, cgi, server, cookie, application, session, url, form,
                    info->capturedScope, effectiveArgs, effectiveArgc);
    } catch (...) {
        while (g_udfCtx.size() > ctxSave) g_udfCtx.pop_back();
        throw;
    }
    while (g_udfCtx.size() > ctxSave) g_udfCtx.pop_back();
    return ret;
}

// Returns the CF-visible keys of an `arguments` struct: the named parameter
// keys (in declaration order) plus numeric keys for arguments beyond the
// parameter count. Numeric keys that duplicate parameter positions are hidden,
// matching CF's StructKeyList(arguments) output (e.g. "A,B" or "A,B,3").
std::vector<webstrada::string> cfml::argumentsVisibleKeys(const cfvariant *arguments)
{
    std::vector<webstrada::string> keys;
    if (!arguments || arguments->m_type != cfvariant::Struct) return keys;
    const std::vector<webstrada::string> *order = arguments->m_structInsertOrder;
    if (!order) return keys;
    int paramCount = arguments->m_argumentsParamCount;
    for (const auto &k : *order) {
        bool numeric = !k.isEmpty();
        for (size_t i = 0; i < k.length(); i++) {
            if (!(k.at(i) >= '0' && k.at(i) <= '9')) { numeric = false; break; }
        }
        if (numeric && k.length() <= 9) {
            long long n = 0;
            for (size_t i = 0; i < k.length(); i++) n = n * 10 + (k.at(i) - '0');
            if (n >= 1 && n <= paramCount) continue;
        }
        keys.push_back(k);
    }
    return keys;
}




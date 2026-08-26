#include "core_internal.h"
#include "../cftags/common.h"

#include <webstrada/cf8.h>
#include <webstrada/component.h>
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

// ---- Unimplemented function stubs ----

cfvariant *cfml::cf_addsoaprequestheader() {
    throw webstrada::exception("Function AddSOAPRequestHeader is not implemented");
}

cfvariant *cfml::cf_addsoapresponseheader() {
    throw webstrada::exception("Function AddSOAPResponseHeader is not implemented");
}

cfvariant *cfml::cf_authenticatedcontext() {
    throw webstrada::exception("Function AuthenticatedContext is not implemented");
}

cfvariant *cfml::cf_authenticateduser() {
    throw webstrada::exception("Function AuthenticatedUser is not implemented");
}

cfvariant *cfml::cf_createencryptedjwt() {
    throw webstrada::exception("Function CreateEncryptedJWT is not implemented");
}

cfvariant *cfml::cf_createobject(const cfvariant **args, int argc,
                                 string &out, void *cgi, void *server, void *cookie,
                                 void *application, void *session, void *url, void *form,
                                 void *variables) {
    if (argc < 2) throw webstrada::exception("CreateObject requires 2 arguments");
    string typeStr = const_cast<cfvariant*>(args[0])->toString();
    string t = typeStr;
    t.toLower();
    if (t.equals("component")) {
        string path = const_cast<cfvariant*>(args[1])->toString();
        if (std::getenv("WEBSTRADA_DEBUG_COMPONENTS")) {
            IncludeRuntime *rt = cfml::include_context();
            fprintf(stderr, "[WebStrada][DebugComponent] CreateObject type=component requested='%s' currentPath='%s' webRoot='%s'\n",
                    path.constData() ? path.constData() : "",
                    rt ? rt->currentPath.c_str() : "",
                    rt ? rt->webRoot.c_str() : "");
            fflush(stderr);
        }
        ComponentInfo *info = cf_component_load(path.constData());
        if (!info) {
            std::string p = path.constData() ? path.constData() : "";
            throw webstrada::exception("component",
                webstrada::string(("The component " + p + " could not be found.").c_str()));
        }
        if (std::getenv("WEBSTRADA_DEBUG_COMPONENTS")) {
            fprintf(stderr, "[WebStrada][DebugComponent] CreateObject loaded cfcPath='%s' fullName='%s' displayPath='%s'\n",
                    info->cfcPath.c_str(), info->fullName.c_str(), info->displayPath.c_str());
            fflush(stderr);
        }
        // The loaded info is retained; release it even when instantiation throws
        // (e.g. instantiating an interface), so the definition is not leaked.
        struct InfoGuard {
            ComponentInfo *i;
            ~InfoGuard() { if (i) cf_component_info_release(i); }
        } infoGuard{info};
        cfvariant *inst = cf_component_instantiate(info, static_cast<cfvariant*>(variables),
                                                   &out, cgi, server, cookie, application, session, url, form);
        infoGuard.i = nullptr;
        cf_component_info_release(info);  // drop the loader retain (cache holds its ref)
        return inst;
    }
    throw webstrada::exception(webstrada::string(("CreateObject does not support the type " + std::string(typeStr.constData() ? typeStr.constData() : "") + ".").c_str()));
}

cfvariant *cfml::cf_createsignedjwt() {
    throw webstrada::exception("Function CreateSignedJWT is not implemented");
}

cfvariant *cfml::cf_dotnettocftype() {
    throw webstrada::exception("Function DotNetToCFType is not implemented");
}

cfvariant *cfml::cf_entitydelete() {
    throw webstrada::exception("Function EntityDelete is not implemented");
}

cfvariant *cfml::cf_entityload() {
    throw webstrada::exception("Function EntityLoad is not implemented");
}

cfvariant *cfml::cf_entityloadbyexample() {
    throw webstrada::exception("Function EntityLoadByExample is not implemented");
}

cfvariant *cfml::cf_entityloadbypk() {
    throw webstrada::exception("Function EntityLoadByPK is not implemented");
}

cfvariant *cfml::cf_entitymerge() {
    throw webstrada::exception("Function EntityMerge is not implemented");
}

cfvariant *cfml::cf_entitynew() {
    throw webstrada::exception("Function EntityNew is not implemented");
}

cfvariant *cfml::cf_entityreload() {
    throw webstrada::exception("Function EntityReload is not implemented");
}

cfvariant *cfml::cf_entitysave() {
    throw webstrada::exception("Function EntitySave is not implemented");
}

cfvariant *cfml::cf_entitytoquery() {
    throw webstrada::exception("Function EntityToQuery is not implemented");
}

cfvariant *cfml::cf_generatesamlspmetadata() {
    throw webstrada::exception("Function GenerateSAMLSPMetadata is not implemented");
}


cfvariant *cfml::cf_getcomponentmetadata(const cfvariant *obj) {
    if (!obj) throw webstrada::exception("GetComponentMetaData requires 1 argument");
    return cf_getcomponentmetadata_impl(obj);
}

cfvariant *cfml::cf_getfunctioncalledname() {
    throw webstrada::exception("Function GetFunctionCalledName is not implemented");
}

cfvariant *cfml::cf_getgatewayhelper() {
    throw webstrada::exception("Function GetGatewayHelper is not implemented");
}

cfvariant *cfml::cf_getk2serverdoccount() {
    throw webstrada::exception("Function GetK2ServerDocCount is not implemented");
}

cfvariant *cfml::cf_getk2serverdoccountlimit() {
    throw webstrada::exception("Function GetK2ServerDocCountLimit is not implemented");
}

cfvariant *cfml::cf_getpagecontext() {
    throw webstrada::exception("Function GetPageContext is not implemented");
}

cfvariant *cfml::cf_getprinterinfo() {
    throw webstrada::exception("Function GetPrinterInfo is not implemented");
}

cfvariant *cfml::cf_getprinterlist() {
    throw webstrada::exception("Function GetPrinterList is not implemented");
}

cfvariant *cfml::cf_getsafehtml() {
    throw webstrada::exception("Function GetSafeHTML is not implemented");
}

cfvariant *cfml::cf_getsamlauthrequest() {
    throw webstrada::exception("Function GetSAMLAuthRequest is not implemented");
}

cfvariant *cfml::cf_getsamllogoutrequest() {
    throw webstrada::exception("Function GetSAMLLogoutRequest is not implemented");
}

cfvariant *cfml::cf_getsoaprequest() {
    throw webstrada::exception("Function GetSOAPRequest is not implemented");
}

cfvariant *cfml::cf_getsoaprequestheader() {
    throw webstrada::exception("Function GetSOAPRequestHeader is not implemented");
}

cfvariant *cfml::cf_getsoapresponse() {
    throw webstrada::exception("Function GetSOAPResponse is not implemented");
}

cfvariant *cfml::cf_getsoapresponseheader() {
    throw webstrada::exception("Function GetSOAPResponseHeader is not implemented");
}

cfvariant *cfml::cf_getvfsmetadata() {
    throw webstrada::exception("Function GetVFSMetaData is not implemented");
}

cfvariant *cfml::cf_hqlmethods() {
    throw webstrada::exception("Function HQLMethods is not implemented");
}

cfvariant *cfml::cf_initsamlauthrequest() {
    throw webstrada::exception("Function InitSAMLAuthRequest is not implemented");
}

cfvariant *cfml::cf_initsamllogoutrequest() {
    throw webstrada::exception("Function InitSAMLLogoutRequest is not implemented");
}

cfvariant *cfml::cf_interruptthread() {
    throw webstrada::exception("Function InterruptThread is not implemented");
}

cfvariant *cfml::cf_invalidateoauthaccesstoken() {
    throw webstrada::exception("Function InvalidateOauthAccesstoken is not implemented");
}

cfvariant *cfml::cf_isauthenticated() {
    throw webstrada::exception("Function IsAuthenticated is not implemented");
}

cfvariant *cfml::cf_isauthorized() {
    throw webstrada::exception("Function IsAuthorized is not implemented");
}

cfvariant *cfml::cf_isinstanceof(const cfvariant *obj, const cfvariant *typeName) {
    if (!obj || !typeName) throw webstrada::exception("IsInstanceOf requires 2 arguments");
    return cf_isinstanceof_impl(obj, typeName);
}

cfvariant *cfml::cf_isk2serverabroker() {
    throw webstrada::exception("Function IsK2ServerABroker is not implemented");
}

cfvariant *cfml::cf_isk2serverdoccountexceeded() {
    throw webstrada::exception("Function IsK2ServerDocCountExceeded is not implemented");
}

cfvariant *cfml::cf_isk2serveronline() {
    throw webstrada::exception("Function IsK2ServerOnline is not implemented");
}

cfvariant *cfml::cf_isprotected() {
    throw webstrada::exception("Function IsProtected is not implemented");
}

cfvariant *cfml::cf_issafehtml() {
    throw webstrada::exception("Function isSafeHTML is not implemented");
}

cfvariant *cfml::cf_issamllogoutresponse() {
    throw webstrada::exception("Function isSamlLogoutResponse is not implemented");
}

cfvariant *cfml::cf_issoaprequest() {
    throw webstrada::exception("Function IsSOAPRequest is not implemented");
}

cfvariant *cfml::cf_isspreadsheetfile() {
    throw webstrada::exception("Function IsSpreadsheetFile is not implemented");
}

cfvariant *cfml::cf_isspreadsheetobject() {
    throw webstrada::exception("Function IsSpreadsheetObject is not implemented");
}

cfvariant *cfml::cf_isvalidoauthaccesstoken() {
    throw webstrada::exception("Function IsValidOauthAccesstoken is not implemented");
}

cfvariant *cfml::cf_javacast() {
    throw webstrada::exception("Function JavaCast is not implemented");
}

cfvariant *cfml::cf_onwsauthenticate() {
    throw webstrada::exception("Function OnWSAuthenticate is not implemented");
}

cfvariant *cfml::cf_ormclearsession() {
    throw webstrada::exception("Function ORMClearSession is not implemented");
}

cfvariant *cfml::cf_ormcloseallsessions() {
    throw webstrada::exception("Function ORMCloseAllSessions is not implemented");
}

cfvariant *cfml::cf_ormclosesession() {
    throw webstrada::exception("Function ORMCloseSession is not implemented");
}

cfvariant *cfml::cf_ormevictcollection() {
    throw webstrada::exception("Function ORMEvictCollection is not implemented");
}

cfvariant *cfml::cf_ormevictentity() {
    throw webstrada::exception("Function ORMEvictEntity is not implemented");
}

cfvariant *cfml::cf_ormevictqueries() {
    throw webstrada::exception("Function ORMEvictQueries is not implemented");
}

cfvariant *cfml::cf_ormexecutequery() {
    throw webstrada::exception("Function ORMExecuteQuery is not implemented");
}

cfvariant *cfml::cf_ormflush() {
    throw webstrada::exception("Function ORMFlush is not implemented");
}

cfvariant *cfml::cf_ormflushall() {
    throw webstrada::exception("Function ORMFlushall is not implemented");
}

cfvariant *cfml::cf_ormgetsession() {
    throw webstrada::exception("Function ORMGetSession is not implemented");
}

cfvariant *cfml::cf_ormgetsessionfactory() {
    throw webstrada::exception("Function ORMGetSessionFactory is not implemented");
}

cfvariant *cfml::cf_ormindex() {
    throw webstrada::exception("Function ORMIndex is not implemented");
}

cfvariant *cfml::cf_ormindexpurge() {
    throw webstrada::exception("Function ORMIndexPurge is not implemented");
}

cfvariant *cfml::cf_ormreload() {
    throw webstrada::exception("Function ORMReload is not implemented");
}

cfvariant *cfml::cf_ormsearch() {
    throw webstrada::exception("Function ORMSearch is not implemented");
}

cfvariant *cfml::cf_ormsearchoffline() {
    throw webstrada::exception("Function ORMSearchOffline is not implemented");
}

cfvariant *cfml::cf_processsamllogoutrequest() {
    throw webstrada::exception("Function ProcessSAMLLogoutRequest is not implemented");
}

cfvariant *cfml::cf_processsamlresponse() {
    throw webstrada::exception("Function ProcessSAMLResponse is not implemented");
}

cfvariant *cfml::cf_releasecomobject() {
    throw webstrada::exception("Function ReleaseComObject is not implemented");
}

cfvariant *cfml::cf_restdeleteapplication() {
    throw webstrada::exception("Function RestDeleteApplication is not implemented");
}

cfvariant *cfml::cf_restinitapplication() {
    throw webstrada::exception("Function RestInitApplication is not implemented");
}

cfvariant *cfml::cf_restsetresponse() {
    throw webstrada::exception("Function RestSetResponse is not implemented");
}

cfvariant *cfml::cf_sendgatewaymessage() {
    throw webstrada::exception("Function SendGatewayMessage is not implemented");
}

cfvariant *cfml::cf_sendsamllogoutresponse() {
    throw webstrada::exception("Function SendSAMLLogoutResponse is not implemented");
}

cfvariant *cfml::cf_setencoding() {
    throw webstrada::exception("Function SetEncoding is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddautofilter() {
    throw webstrada::exception("Function SpreadsheetAddAutoFilter is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddcolumn() {
    throw webstrada::exception("Function SpreadsheetAddColumn is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddfreezepane() {
    throw webstrada::exception("Function SpreadsheetAddFreezePane is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddimage() {
    throw webstrada::exception("Function SpreadsheetAddImage is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddinfo() {
    throw webstrada::exception("Function SpreadsheetAddInfo is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddpagebreaks() {
    throw webstrada::exception("Function SpreadsheetAddPageBreaks is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddprintgridlines() {
    throw webstrada::exception("Function SpreadsheetAddPrintGridlines is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddrow() {
    throw webstrada::exception("Function SpreadsheetAddRow is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddrows() {
    throw webstrada::exception("Function SpreadsheetAddRows is not implemented");
}

cfvariant *cfml::cf_spreadsheetaddsplitpane() {
    throw webstrada::exception("Function SpreadsheetAddSplitPane is not implemented");
}

cfvariant *cfml::cf_spreadsheetcreatesheet() {
    throw webstrada::exception("Function SpreadsheetCreateSheet is not implemented");
}

cfvariant *cfml::cf_spreadsheetdeletecolumn() {
    throw webstrada::exception("Function SpreadsheetDeleteColumn is not implemented");
}

cfvariant *cfml::cf_spreadsheetdeletecolumns() {
    throw webstrada::exception("Function SpreadsheetDeleteColumns is not implemented");
}

cfvariant *cfml::cf_spreadsheetdeleterow() {
    throw webstrada::exception("Function SpreadsheetDeleteRow is not implemented");
}

cfvariant *cfml::cf_spreadsheetdeleterows() {
    throw webstrada::exception("Function SpreadsheetDeleteRows is not implemented");
}

cfvariant *cfml::cf_spreadsheetformatcell() {
    throw webstrada::exception("Function SpreadsheetFormatCell is not implemented");
}

cfvariant *cfml::cf_spreadsheetformatcellrange() {
    throw webstrada::exception("Function SpreadsheetFormatCellRange is not implemented");
}

cfvariant *cfml::cf_spreadsheetformatcolumn() {
    throw webstrada::exception("Function SpreadsheetFormatColumn is not implemented");
}

cfvariant *cfml::cf_spreadsheetformatcolumns() {
    throw webstrada::exception("Function SpreadsheetFormatColumns is not implemented");
}

cfvariant *cfml::cf_spreadsheetformatrow() {
    throw webstrada::exception("Function SpreadsheetFormatRow is not implemented");
}

cfvariant *cfml::cf_spreadsheetformatrows() {
    throw webstrada::exception("Function SpreadsheetFormatRows is not implemented");
}

cfvariant *cfml::cf_spreadsheetgetcellcomment() {
    throw webstrada::exception("Function SpreadsheetGetCellComment is not implemented");
}

cfvariant *cfml::cf_spreadsheetgetcellformula() {
    throw webstrada::exception("Function SpreadsheetGetCellFormula is not implemented");
}

cfvariant *cfml::cf_spreadsheetgetcellvalue() {
    throw webstrada::exception("Function SpreadsheetGetCellValue is not implemented");
}

cfvariant *cfml::cf_spreadsheetgetcolumncount() {
    throw webstrada::exception("Function SpreadsheetGetColumnCount is not implemented");
}

cfvariant *cfml::cf_spreadsheetgetcolumnwidth() {
    throw webstrada::exception("Function SpreadsheetGetColumnWidth is not implemented");
}

cfvariant *cfml::cf_spreadsheetgetlastrownumber() {
    throw webstrada::exception("Function SpreadsheetGetLastRowNumber is not implemented");
}

cfvariant *cfml::cf_spreadsheetgetprintorientation() {
    throw webstrada::exception("Function SpreadsheetGetPrintOrientation is not implemented");
}

cfvariant *cfml::cf_spreadsheetgroupcolumns() {
    throw webstrada::exception("Function SpreadsheetGroupColumns is not implemented");
}

cfvariant *cfml::cf_spreadsheetgrouprows() {
    throw webstrada::exception("Function SpreadsheetGroupRows is not implemented");
}

cfvariant *cfml::cf_spreadsheetinfo() {
    throw webstrada::exception("Function SpreadsheetInfo is not implemented");
}

cfvariant *cfml::cf_spreadsheetisbinaryformat() {
    throw webstrada::exception("Function SpreadsheetisBinaryFormat is not implemented");
}

cfvariant *cfml::cf_spreadsheetiscolumnhidden() {
    throw webstrada::exception("Function SpreadsheetisColumnHidden is not implemented");
}

cfvariant *cfml::cf_spreadsheetisrowhidden() {
    throw webstrada::exception("Function SpreadsheetisRowHidden is not implemented");
}

cfvariant *cfml::cf_spreadsheetisstreamingxmlformat() {
    throw webstrada::exception("Function SpreadsheetisStreamingXmlFormat is not implemented");
}

cfvariant *cfml::cf_spreadsheetisxmlformat() {
    throw webstrada::exception("Function SpreadsheetisXmlFormat is not implemented");
}

cfvariant *cfml::cf_spreadsheetmergecells() {
    throw webstrada::exception("Function SpreadsheetMergeCells is not implemented");
}

cfvariant *cfml::cf_spreadsheetnew() {
    throw webstrada::exception("Function SpreadsheetNew is not implemented");
}

cfvariant *cfml::cf_spreadsheetread() {
    throw webstrada::exception("Function SpreadsheetRead is not implemented");
}

cfvariant *cfml::cf_spreadsheetreadbinary() {
    throw webstrada::exception("Function SpreadsheetReadBinary is not implemented");
}

cfvariant *cfml::cf_spreadsheetremovecolumnbreak() {
    throw webstrada::exception("Function SpreadsheetRemoveColumnBreak is not implemented");
}

cfvariant *cfml::cf_spreadsheetremoveprintgridlines() {
    throw webstrada::exception("Function SpreadsheetRemovePrintGridlines is not implemented");
}

cfvariant *cfml::cf_spreadsheetremoverowbreak() {
    throw webstrada::exception("Function SpreadsheetRemoveRowBreak is not implemented");
}

cfvariant *cfml::cf_spreadsheetremovesheet() {
    throw webstrada::exception("Function SpreadsheetRemoveSheet is not implemented");
}

cfvariant *cfml::cf_spreadsheetremovesheetnumber() {
    throw webstrada::exception("Function SpreadsheetRemoveSheetNumber is not implemented");
}

cfvariant *cfml::cf_spreadsheetrenamesheet() {
    throw webstrada::exception("Function SpreadsheetRenameSheet is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetactivesheet() {
    throw webstrada::exception("Function SpreadsheetSetActiveSheet is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetactivesheetnumber() {
    throw webstrada::exception("Function SpreadsheetSetActiveSheetNumber is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetcellcomment() {
    throw webstrada::exception("Function SpreadsheetSetCellComment is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetcellformula() {
    throw webstrada::exception("Function SpreadsheetSetCellFormula is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetcellvalue() {
    throw webstrada::exception("Function SpreadsheetSetCellValue is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetcolumnbreak() {
    throw webstrada::exception("Function SpreadsheetSetColumnBreak is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetcolumnhidden() {
    throw webstrada::exception("Function SpreadsheetSetColumnHidden is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetcolumnwidth() {
    throw webstrada::exception("Function SpreadsheetSetColumnWidth is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetfittopage() {
    throw webstrada::exception("Function SpreadsheetSetFittoPage is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetfooter() {
    throw webstrada::exception("Function SpreadsheetSetFooter is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetfooterimage() {
    throw webstrada::exception("Function SpreadsheetSetFooterImage is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetheader() {
    throw webstrada::exception("Function SpreadsheetSetHeader is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetheaderimage() {
    throw webstrada::exception("Function SpreadsheetSetHeaderImage is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetrowbreak() {
    throw webstrada::exception("Function SpreadsheetSetRowBreak is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetrowheight() {
    throw webstrada::exception("Function SpreadsheetSetRowHeight is not implemented");
}

cfvariant *cfml::cf_spreadsheetsetrowhidden() {
    throw webstrada::exception("Function SpreadsheetSetRowHidden is not implemented");
}

cfvariant *cfml::cf_spreadsheetshiftcolumns() {
    throw webstrada::exception("Function SpreadsheetShiftColumns is not implemented");
}

cfvariant *cfml::cf_spreadsheetshiftrows() {
    throw webstrada::exception("Function SpreadsheetShiftRows is not implemented");
}

cfvariant *cfml::cf_spreadsheetungroupcolumns() {
    throw webstrada::exception("Function SpreadsheetUngroupColumns is not implemented");
}

cfvariant *cfml::cf_spreadsheetungrouprows() {
    throw webstrada::exception("Function SpreadsheetUngroupRows is not implemented");
}

cfvariant *cfml::cf_spreadsheetwrite() {
    throw webstrada::exception("Function SpreadsheetWrite is not implemented");
}

cfvariant *cfml::cf_storeaddacl() {
    throw webstrada::exception("Function StoreAddACL is not implemented");
}

cfvariant *cfml::cf_storegetacl() {
    throw webstrada::exception("Function StoreGetACL is not implemented");
}

cfvariant *cfml::cf_storegetmetadata() {
    throw webstrada::exception("Function StoreGetMetadata is not implemented");
}

cfvariant *cfml::cf_storesetacl() {
    throw webstrada::exception("Function StoreSetACL is not implemented");
}

cfvariant *cfml::cf_storesetmetadata() {
    throw webstrada::exception("Function StoreSetMetadata is not implemented");
}

cfvariant *cfml::cf_streamingspreadsheetcleanup() {
    throw webstrada::exception("Function StreamingSpreadsheetCleanup is not implemented");
}

cfvariant *cfml::cf_streamingspreadsheetisstreamingxmlformat() {
    throw webstrada::exception("Function StreamingSpreadsheetIsStreamingXmlFormat is not implemented");
}

cfvariant *cfml::cf_streamingspreadsheetisxmlformat() {
    throw webstrada::exception("Function StreamingSpreadsheetIsXmlFormat is not implemented");
}

cfvariant *cfml::cf_streamingspreadsheetnew() {
    throw webstrada::exception("Function StreamingSpreadsheetNew is not implemented");
}

cfvariant *cfml::cf_streamingspreadsheetprocess() {
    throw webstrada::exception("Function StreamingSpreadsheetProcess is not implemented");
}

cfvariant *cfml::cf_streamingspreadsheetread() {
    throw webstrada::exception("Function StreamingSpreadsheetRead is not implemented");
}

cfvariant *cfml::cf_threadjoin() {
    throw webstrada::exception("Function ThreadJoin is not implemented");
}

cfvariant *cfml::cf_threadterminate() {
    throw webstrada::exception("Function ThreadTerminate is not implemented");
}

cfvariant *cfml::cf_throw() {
    throw webstrada::exception("Function Throw is not implemented");
}

cfvariant *cfml::cf_verifyclient() {
    throw webstrada::exception("Function VerifyClient is not implemented");
}

cfvariant *cfml::cf_wsgetallchannels() {
    throw webstrada::exception("Function WSGetAllChannels is not implemented");
}

cfvariant *cfml::cf_wsgetsubscribers() {
    throw webstrada::exception("Function WSGetSubscribers is not implemented");
}

cfvariant *cfml::cf_wspublish() {
    throw webstrada::exception("Function WSPublish is not implemented");
}

cfvariant *cfml::cf_wssendmessage() {
    throw webstrada::exception("Function WSSendMessage is not implemented");
}

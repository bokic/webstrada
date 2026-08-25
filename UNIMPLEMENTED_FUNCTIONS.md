# Unimplemented CFML Functions

Total: 161 functions (24.17% of 666) that are still unimplemented and throw the `Function X is not implemented` stub error. This covers everything marked `❌ No` in PROGRESS.md (CFML Functions table).

## Unimplemented

| Category | Count | % of total | Functions |
|---|---|---|---|
| **Auth/SAML/OAuth** | 18 | 11.18% | AuthenticatedContext, AuthenticatedUser, GenerateSAMLSPMetadata, GetSAMLAuthRequest, GetSAMLLogoutRequest, GetSafeHTML, InitSAMLAuthRequest, InitSAMLLogoutRequest, InvalidateOauthAccesstoken, IsAuthenticated, IsAuthorized, IsValidOauthAccesstoken, ProcessSAMLLogoutRequest, ProcessSAMLResponse, SendSAMLLogoutResponse, VerifyClient, isSafeHTML, isSamlLogoutResponse |
| **Call/Tag stack** | 1 | 0.61% | GetFunctionCalledName |
| **Get*/Meta/System** | 8 | 4.76% | GetGatewayHelper, GetK2ServerDocCount, GetK2ServerDocCountLimit, GetPageContext, GetPrinterInfo, GetPrinterList, GetVFSMetaData, SetEncoding (deferred) |
| **Crypto/Token/Decision** | 6 | 3.57% | CreateEncryptedJWT, CreateSignedJWT (deferred: JKS keystore needed), IsK2ServerABroker, IsK2ServerDocCountExceeded, IsK2ServerOnline, IsProtected |
| **Create/Misc** | 9 | 5.36% | InterruptThread, SendGatewayMessage, StoreAddACL, StoreGetACL, StoreGetMetadata, StoreSetACL, StoreSetMetadata, ThreadJoin, ThreadTerminate |
| **Spreadsheet*** | 75 | 44.64% | IsSpreadsheetFile, IsSpreadsheetObject, SpreadsheetAddAutoFilter, SpreadsheetAddColumn, SpreadsheetAddFreezePane, SpreadsheetAddImage, SpreadsheetAddInfo, SpreadsheetAddPageBreaks, SpreadsheetAddPrintGridlines, SpreadsheetAddRow, SpreadsheetAddRows, SpreadsheetAddSplitPane, SpreadsheetCreateSheet, SpreadsheetDeleteColumn, SpreadsheetDeleteColumns, SpreadsheetDeleteRow, SpreadsheetDeleteRows, SpreadsheetFormatCell, SpreadsheetFormatCellRange, SpreadsheetFormatColumn, SpreadsheetFormatColumns, SpreadsheetFormatRow, SpreadsheetFormatRows, SpreadsheetGetCellComment, SpreadsheetGetCellFormula, SpreadsheetGetCellValue, SpreadsheetGetColumnCount, SpreadsheetGetColumnWidth, SpreadsheetGetLastRowNumber, SpreadsheetGetPrintOrientation, SpreadsheetGroupColumns, SpreadsheetGroupRows, SpreadsheetInfo, SpreadsheetMergeCells, SpreadsheetNew, SpreadsheetRead, SpreadsheetReadBinary, SpreadsheetRemoveColumnBreak, SpreadsheetRemovePrintGridlines, SpreadsheetRemoveRowBreak, SpreadsheetRemoveSheet, SpreadsheetRemoveSheetNumber, SpreadsheetRenameSheet, SpreadsheetSetActiveSheet, SpreadsheetSetActiveSheetNumber, SpreadsheetSetCellComment, SpreadsheetSetCellFormula, SpreadsheetSetCellValue, SpreadsheetSetColumnBreak, SpreadsheetSetColumnHidden, SpreadsheetSetColumnWidth, SpreadsheetSetFittoPage, SpreadsheetSetFooter, SpreadsheetSetFooterImage, SpreadsheetSetHeader, SpreadsheetSetHeaderImage, SpreadsheetSetRowBreak, SpreadsheetSetRowHeight, SpreadsheetSetRowHidden, SpreadsheetShiftColumns, SpreadsheetShiftRows, SpreadsheetUngroupColumns, SpreadsheetUngroupRows, SpreadsheetWrite, SpreadsheetisBinaryFormat, SpreadsheetisColumnHidden, SpreadsheetisRowHidden, SpreadsheetisStreamingXmlFormat, SpreadsheetisXmlFormat, StreamingSpreadsheetCleanup, StreamingSpreadsheetIsStreamingXmlFormat, StreamingSpreadsheetIsXmlFormat, StreamingSpreadsheetNew, StreamingSpreadsheetProcess, StreamingSpreadsheetRead |
| **Java/.NET objects** | 3 | 1.79% | DotNetToCFType, JavaCast, ReleaseComObject |
| **ORM/Entity** | 26 | 15.48% | EntityDelete, EntityLoad, EntityLoadByExample, EntityLoadByPK, EntityMerge, EntityNew, EntityReload, EntitySave, EntityToQuery, HQLMethods, ORMClearSession, ORMCloseAllSessions, ORMCloseSession, ORMEvictCollection, ORMEvictEntity, ORMEvictQueries, ORMExecuteQuery, ORMFlush, ORMFlushall, ORMGetSession, ORMGetSessionFactory, ORMIndex, ORMIndexPurge, ORMReload, ORMSearch, ORMSearchOffline |
| **SOAP/WS** | 12 | 7.14% | AddSOAPRequestHeader, AddSOAPResponseHeader, GetSOAPRequest, GetSOAPRequestHeader, GetSOAPResponse, GetSOAPResponseHeader, IsSOAPRequest, OnWSAuthenticate, WSGetAllChannels, WSGetSubscribers, WSPublish, WSSendMessage |
| **REST** | 3 | 1.79% | RestDeleteApplication, RestInitApplication, RestSetResponse |

Notes:
* The implemented date mutators (`SetYear`, `SetMonth`, `SetDay`, `SetHour`,
  `SetMinute`, and `SetSecond`) retain their two-argument built-in behavior at
  page level, while bare calls with the same names inside component methods
  resolve the component method first (see `ComponentTest.BareDateMutatorNameResolvesComponentMethod`).
* InvokeCFClientFunction is **not a ColdFusion 2025 function** — CF reports `Variable INVOKECFCLIENTFUNCTION is undefined.`, which the engine reproduces (see fn_ajax.cpp).
* The **cflogin model** functions (GetAuthUser, GetUserRoles, IsUserLoggedIn, IsUserInRole, IsUserInAnyRole) were implemented on 2026-08-11 with the `<cflogin>`/`<cfloginuser>`/`<cflogout>` tags (see PROGRESS.md).
Implementation note: direct custom-tag syntax (`<cf_name>`) is a compiler/tag feature and adds no CFML function.

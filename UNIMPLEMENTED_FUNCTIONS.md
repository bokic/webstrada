# Unimplemented CFML Functions

Compatibility note (2026-09-07): `IsProtected`, `IsAuthenticated`, and `IsAuthorized`
are obsolete ColdFusion security functions (removed in CF MX / CF 6). In modern
ColdFusion, CF reports `Variable <NAME> is undefined.`, which the engine reproduces
(see fn_isauthenticated.cpp, fn_isauthorized.cpp, fn_isprotected.cpp); UDFs with
these names are allowed.

Compatibility note (2026-08-27): `<cfmail>` now runs as a logging stub; no
function implementation status changed.

Compatibility note (2026-08-26): quoted string keys in named arguments are
now supported; no function implementation status changed.

Compatibility note (2026-08-26): Mango_2_1 author-creation call sites were
corrected to place the role in argument 7 and pass `active=true` in argument 8;
no function implementation status changed.

Compatibility note (2026-08-26): typed Boolean UDF argument conversion now
matches Adobe ColdFusion's argument-validation exception message; no function
implementation status changed.

Compatibility note (2026-08-26): CFC indexed-assignment member creation was
fixed as an execution/code-generation behavior; no CFML function status
changed.

Compatibility note (2026-08-26): script-form methods nested in tag-based CFCs
are now registered for component dispatch; no CFML function status changed.

Compatibility note (2026-08-26): script-form method declarations in tag-based
CFCs are removed from the executable `<cfscript>` stream after hoisting; this
is a code-generation fix and does not change the function implementation list.

Compatibility note (2026-08-26): dotted assignments in tag-based component
methods now resolve and persist through the live `this` scope; no CFML function
status changed.

Compatibility note (2026-08-26): cfscript `new` class-name token handling was
corrected for component construction; no CFML function status changed.

Compatibility note (2026-08-26): nested save-content output expressions now
preserve the active component method context; this execution fix does not add
or remove a CFML function from the unimplemented list.

Compatibility note (2026-08-26): the per-worker compilation cache checks source
modification times for all `.cfm` templates and `.cfc` components, including
`Application.cfm`/`Application.cfc`; changed components are recompiled before
the next load.

Compatibility note (2026-08-26): custom-tag calls now preserve component/UDF
local assignment precedence; this is an execution-scope fix and does not add
or remove a CFML function from the unimplemented list.

Total: 157 functions (23.57% of 666) that are still unimplemented and throw the `Function X is not implemented` stub error. This covers everything marked `❌ No` in PROGRESS.md (CFML Functions table).

## Unimplemented

| Category | Count | % of total | Functions |
|---|---|---|---|
| **Auth/SAML/OAuth** | 16 | 10.19% | AuthenticatedContext, AuthenticatedUser, GenerateSAMLSPMetadata, GetSAMLAuthRequest, GetSAMLLogoutRequest, GetSafeHTML, InitSAMLAuthRequest, InitSAMLLogoutRequest, InvalidateOauthAccesstoken, IsValidOauthAccesstoken, ProcessSAMLLogoutRequest, ProcessSAMLResponse, SendSAMLLogoutResponse, VerifyClient, isSafeHTML, isSamlLogoutResponse |
| **Get*/Meta/System** | 8 | 5.10% | GetGatewayHelper, GetK2ServerDocCount, GetK2ServerDocCountLimit, GetPageContext, GetPrinterInfo, GetPrinterList, GetVFSMetaData, SetEncoding (deferred) |
| **Crypto/Token/Decision** | 5 | 3.18% | CreateEncryptedJWT, CreateSignedJWT (deferred: JKS keystore needed), IsK2ServerABroker, IsK2ServerDocCountExceeded, IsK2ServerOnline |
| **Create/Misc** | 9 | 5.73% | InterruptThread, SendGatewayMessage, StoreAddACL, StoreGetACL, StoreGetMetadata, StoreSetACL, StoreSetMetadata, ThreadJoin, ThreadTerminate |
| **Spreadsheet*** | 75 | 47.77% | IsSpreadsheetFile, IsSpreadsheetObject, SpreadsheetAddAutoFilter, SpreadsheetAddColumn, SpreadsheetAddFreezePane, SpreadsheetAddImage, SpreadsheetAddInfo, SpreadsheetAddPageBreaks, SpreadsheetAddPrintGridlines, SpreadsheetAddRow, SpreadsheetAddRows, SpreadsheetAddSplitPane, SpreadsheetCreateSheet, SpreadsheetDeleteColumn, SpreadsheetDeleteColumns, SpreadsheetDeleteRow, SpreadsheetDeleteRows, SpreadsheetFormatCell, SpreadsheetFormatCellRange, SpreadsheetFormatColumn, SpreadsheetFormatColumns, SpreadsheetFormatRow, SpreadsheetFormatRows, SpreadsheetGetCellComment, SpreadsheetGetCellFormula, SpreadsheetGetCellValue, SpreadsheetGetColumnCount, SpreadsheetGetColumnWidth, SpreadsheetGetLastRowNumber, SpreadsheetGetPrintOrientation, SpreadsheetGroupColumns, SpreadsheetGroupRows, SpreadsheetInfo, SpreadsheetMergeCells, SpreadsheetNew, SpreadsheetRead, SpreadsheetReadBinary, SpreadsheetRemoveColumnBreak, SpreadsheetRemovePrintGridlines, SpreadsheetRemoveRowBreak, SpreadsheetRemoveSheet, SpreadsheetRemoveSheetNumber, SpreadsheetRenameSheet, SpreadsheetSetActiveSheet, SpreadsheetSetActiveSheetNumber, SpreadsheetSetCellComment, SpreadsheetSetCellFormula, SpreadsheetSetCellValue, SpreadsheetSetColumnBreak, SpreadsheetSetColumnHidden, SpreadsheetSetColumnWidth, SpreadsheetSetFittoPage, SpreadsheetSetFooter, SpreadsheetSetFooterImage, SpreadsheetSetHeader, SpreadsheetSetHeaderImage, SpreadsheetSetRowBreak, SpreadsheetSetRowHeight, SpreadsheetSetRowHidden, SpreadsheetShiftColumns, SpreadsheetShiftRows, SpreadsheetUngroupColumns, SpreadsheetUngroupRows, SpreadsheetWrite, SpreadsheetisBinaryFormat, SpreadsheetisColumnHidden, SpreadsheetisRowHidden, SpreadsheetisStreamingXmlFormat, SpreadsheetisXmlFormat, StreamingSpreadsheetCleanup, StreamingSpreadsheetIsStreamingXmlFormat, StreamingSpreadsheetIsXmlFormat, StreamingSpreadsheetNew, StreamingSpreadsheetProcess, StreamingSpreadsheetRead |
| **Java/.NET objects** | 3 | 1.91% | DotNetToCFType, JavaCast, ReleaseComObject |
| **ORM/Entity** | 26 | 16.56% | EntityDelete, EntityLoad, EntityLoadByExample, EntityLoadByPK, EntityMerge, EntityNew, EntityReload, EntitySave, EntityToQuery, HQLMethods, ORMClearSession, ORMCloseAllSessions, ORMCloseSession, ORMEvictCollection, ORMEvictEntity, ORMEvictQueries, ORMExecuteQuery, ORMFlush, ORMFlushall, ORMGetSession, ORMGetSessionFactory, ORMIndex, ORMIndexPurge, ORMReload, ORMSearch, ORMSearchOffline |
| **SOAP/WS** | 12 | 7.64% | AddSOAPRequestHeader, AddSOAPResponseHeader, GetSOAPRequest, GetSOAPRequestHeader, GetSOAPResponse, GetSOAPResponseHeader, IsSOAPRequest, OnWSAuthenticate, WSGetAllChannels, WSGetSubscribers, WSPublish, WSSendMessage |
| **REST** | 3 | 1.91% | RestDeleteApplication, RestInitApplication, RestSetResponse |

Notes:
* `StructDelete` is implemented, including deletion from a component's
  struct-compatible `this` scope; it is not part of this unimplemented list.
* The implemented date mutators (`SetYear`, `SetMonth`, `SetDay`, `SetHour`,
  `SetMinute`, and `SetSecond`) retain their two-argument built-in behavior at
  page level, while bare calls with the same names inside component methods
  resolve the component method first (see `ComponentTest.BareDateMutatorNameResolvesComponentMethod`).
* InvokeCFClientFunction is **not a ColdFusion 2025 function** — CF reports `Variable INVOKECFCLIENTFUNCTION is undefined.`, which the engine reproduces (see fn_ajax.cpp).
* IsAuthenticated, IsAuthorized, and IsProtected are **not ColdFusion 2025 functions** (obsolete functions from pre-MX Advanced Security) — CF reports `Variable <NAME> is undefined.`, which the engine reproduces (see fn_isauthenticated.cpp, fn_isauthorized.cpp, fn_isprotected.cpp). User-defined functions with these names are allowed.
* The **cflogin model** functions (GetAuthUser, GetUserRoles, IsUserLoggedIn, IsUserInRole, IsUserInAnyRole) were implemented on 2026-08-11 with the `<cflogin>`/`<cfloginuser>`/`<cflogout>` tags (see PROGRESS.md).
Implementation note: direct custom-tag syntax (`<cf_name>`) is a compiler/tag feature and adds no CFML function.

Compatibility note (2026-08-26): `trace()` is implemented and is not part of
the unimplemented function count. It appends trace data to `cftrace.log` via
the shared WriteLog path; inline/debug-section output and debug-gated abort
behavior remain disabled while `config::debugEnabled` is false.

Compatibility note (2026-08-26): `<cfinclude>` now explicitly propagates the
caller-local scope across compiled template boundaries; no CFML function is
added by this runtime fix.

Compatibility note (2026-08-26): component-method `arguments` and local-scope
lookup now takes precedence over an included template's caller-local scope;
this changes no unimplemented-function coverage.

Compatibility note (2026-08-25): nested CFC member chains are supported by the
JIT; this fix does not add or remove a standalone CFML function.

Compatibility note (2026-08-25): implicit query-column lookup now occurs after
the active UDF's local variables and arguments, so a query column cannot shadow a
local component-method loop index; this fix does not add or remove a standalone
CFML function.

Compatibility note (2026-08-25): nested component method calls preserve their
callee-local scope through numeric loop execution; Queue-style loop indexes no
longer fall back to the caller's local or query scope. This fix does not add or
remove a standalone CFML function.

Compatibility note (2026-08-25): a nested CFC/UDF invoked from a custom tag now
resolves its local and arguments scopes before the custom tag's private variables,
matching Adobe ColdFusion and fixing Queue-style loop indexes in nested tag calls.

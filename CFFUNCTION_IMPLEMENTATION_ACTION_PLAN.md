# CFFUNCTION Implementation Action Plan

Analysis of all unimplemented cffunctions with an implementability plan.
Source of truth: `PROGRESS.md` (CFML Functions table), `UNIMPLEMENTED_FUNCTIONS.md`, `TODO.md`, and the stub registry in `src/core/core_stubs.cpp`.

## Summary

- **185** functions are marked `❌ No` in PROGRESS.md (CFML Functions table). All are registered in `src/codegen/llvm_compiler.cpp` via `AddSymbol` and throw `Function X is not implemented` (stubs in `src/core/core_stubs.cpp`).
- Every implementation is a standalone `cfvariant *cf_<name>(...)` in `src/cffunctions/` following the `fn_trim.cpp` pattern, then registered in the AddSymbol table.
- Per AGENTS.md: every cffunction argument should be `cfvariant *` (or `const cfvariant *`); return type `cfvariant *` or void.

## Available infrastructure

Already-linked libraries: **curl** (HTTP), **openssl** (HMAC/RAND/AES), **sqlite3**, **pcre2**, **libxml2/libxslt**, **cairo/jpeg/zlib** (images).

Reusable runtime code:
- `<cftransaction>` → `cf_transaction_begin/commit/rollback` (`src/cftags/tag_transaction.cpp`)
- `<cflocation>` → `response_redirect` (`src/cftags/tag_location.cpp`)
- `<cfquery>` → `cf_query_begin/cf_query_end` (`src/cftags/tag_query.cpp`)
- `cfvariant_call_function` (dynamic invoke) — `src/core/core_membermethods.cpp`
- exception classes (`abort_exception`, `template_exception`, `webstrada::exception`) — `include/webstrada/exceptions.h`
- JSON serialize/deserialize helpers
- built-in function-name registry `kBuiltinFunctionNames` — `src/core/core_udf.cpp` (backing for `GetFunctionList`)
- tag runtimes: `cf_cfimage`, upload/file helpers, INI via `GetProfileString`

## Deferred by user decision (needs a new subsystem)

| Function | Effort | Deps | Why deferred |
|---|---|---|---|
| GetFunctionCalledName | M | call stack | needs call-frame recording in UDF runtime — **deferred** |
| CallStackGet / CallStackDump | M | call stack | same frame tracking — **deferred** |
| GetBaseTagList / GetBaseTagData | M | tag stack | needs tag-nesting stack — **deferred** |
| SetEncoding | M | request | set form/url decode charset — **deferred** |
| CreateSignedJWT | M | openssl | JWS HS256/384/512 (HMAC exists) — **deferred** (JKS keystore needed) |
| CreateEncryptedJWT | M/L | openssl | JWE (AES/RSA) — **deferred** |

## Tier 3 — Defer / needs a decision (new subsystem or external service)

| Group | Functions | Why deferred |
|---|---|---|
| Auth/Security | AuthenticatedContext, AuthenticatedUser, GetAuthUser, GetUserRoles, IsAuthenticated, IsAuthorized, IsUserInAnyRole, IsUserInRole, IsUserLoggedIn, VerifyClient, IsProtected | needs an auth config/identity model |
| SAML | GenerateSAMLSPMetadata, Get/Init/ProcessSAMLAuthRequest, Get/Init/Process/SendSAMLLogoutRequest/Response, isSamlLogoutResponse | full SAML protocol |
| OAuth | InvalidateOauthAccesstoken, IsValidOauthAccesstoken | OAuth token store |
| SafeHTML | GetSafeHTML, isSafeHTML | OWASP AntiSamy HTML sanitizer (large) |
| K2 search | GetK2ServerDocCount(Limit), IsK2ServerABroker/Online/DocCountExceeded | server-side search index — N/A without K2 |
| Gateways | GetGatewayHelper, SendGatewayMessage | event-gateway subsystem |
| Printers | GetPrinterInfo, GetPrinterList | printing subsystem |
| VFS | GetVFSMetaData | virtual filesystem |
| Threads | isThreadInterrupted, ThreadJoin, ThreadTerminate, InterruptThread | no thread subsystem (`cfthread` unimplemented); needs a named-thread registry + join/terminate/interrupt model |
| Cache | CacheGet, CacheGetAllIds, CacheGetMetadata, CacheGetProperties, CacheGetSession, CacheIdExists, CachePut, CacheRegionExists, CacheRegionNew, CacheRegionRemove, CacheRemove, CacheRemoveAll, CacheSetProperties, RemoveCachedQuery | implementable via sqlite (per TODO.md); needs a cache subsystem/decision | — **implemented 2026-08-08** (sqlite-backed `CacheStore`, `<cfquery cachedwithin/cacheid/cachedafter/cacheregion>` query cache too; see PROGRESS.md). Cannot be byte-verified on the RDS host (no `caching` package, see BUGS_CF.md). |
| PageContext | GetPageContext | returns a Java object — cannot replicate; should throw (AGENTS.md rule) |
| Store ACL | StoreAddACL, StoreGetACL, StoreGetMetadata, StoreSetACL, StoreSetMetadata | web-storage framework (S3 etc.) |
| Spreadsheet* | 75 functions | large subsystem; needs a spreadsheet library (per TODO.md: via xlnt); currently throw stubs |
| ORM/Entity* | 26 functions | needs an ORM subsystem (per TODO.md: custom implementation); currently throw stubs |
| SOAP/WS* | 12 functions | needs SOAP/web-services support (per TODO.md: libcurl/libxml2/gSOAP); currently throw stubs |
| Java/.NET | 3 functions | needs Java/.NET interop (per TODO.md: jnipp / libmono); currently throw stubs |
| REST | 3 functions | needs REST framework integration; currently throw stubs |

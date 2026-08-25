# Known Bugs and Issues (this codebase)

Bugs, behavioral divergences and deliberate limitations in the WebStrada engine itself.
For issues that live on the ColdFusion side (server/installation problems found while
testing against the RDS host) see `BUGS_CF.md`; for cosmetic output artifacts see
`BUGS_COSMETIC.md`.

## `ImageGetBufferedImage` returns the image instead of a `java.awt.image.BufferedImage`

Adobe CF returns the underlying `java.awt.image.BufferedImage` (a live Java object) from `ImageGetBufferedImage()`. This engine has no Java object interop, so it returns the image itself — the closest stand-in for the image's backing store — and `IsImage(ImageGetBufferedImage(img))` is therefore `YES` (CF: `NO`, since a BufferedImage is not a ColdFusion image object; CF's `IsObject` is `YES`, this engine's is `NO`). `ImageGetWidth(ImageGetBufferedImage(img))` works on both. Kept as a documented divergence (see `src/cfimage_meta.cpp`).

## EXIF makernotes are not reported

`ImageGetEXIFMetadata` reproduces com.drew's IFD0 / SubIFD / GPS / Interop / Thumbnail tag processing but does not parse vendor makernote directories (Canon, Nikon, Olympus, ...), so images carrying a Makernote (tag 0x927C) with a recognized vendor `Make` will report fewer EXIF keys than CF. The tags the engine does report are byte-verified against CF 2025 (tests/cfm/image_exif_metadata.cfm + image_meta_errors.cfm).

## `GetApplicationMetadata()` returns a subset of ColdFusion's struct

CF 2025's `GetApplicationMetadata()` returns a large struct (RESTSETTINGS, MAPPINGS, DATASOURCES, ORMSETTINGS, SESSIONCOOKIE, SECURITY, SERIALIZATION, SAMLSETTINGS, etc.). This engine returns only NAME, APPLICATIONTIMEOUT, SESSIONMANAGEMENT, SESSIONTIMEOUT, CLIENTMANAGEMENT, SETCLIENTCOOKIES, SETDOMAINCOOKIES, LOGINSTORAGE. The covered keys are byte-verified (tests/cfm/cfapplication_scope_test.cfm); the rest would require an application-settings model (Application.cfc) that does not exist yet. **When no application is active (no `<cfapplication>`/Application.cfc ran), `GetApplicationMetadata()` now returns an EMPTY struct like CF** (AppHelper.getApplicationMetaData returns null and CFPage falls back to a fresh empty Struct — previously this engine returned the 8 keys unconditionally; byte-verified in tests/cfm/getappmetadata_empty_test.cfm).


## `<cfdbinfo>` attribute-validation errors are catchable; CF aborts the request

CF's `<cfdbinfo>` validation errors (an invalid `type`, a missing `name` for types that need one, a missing `table` for columns/foreignkeys/index) are **compile-time** errors: they abort the whole request — no output is rendered and a surrounding `<cftry>`/`<cfcatch>` cannot intercept them (verified against CF 2025 on the RDS host: even `PRE|` output before the tag is not emitted). This engine surfaces the same messages ("Attribute validation error for CFDBINFO. The value of the TYPE attribute, which is currently X, must be one of the values: ..." / "Attribute validation error for tag CFDBINFO. When the value of the TYPE attribute is X, it requires the attribute(s): NAME./TABLE.") but as **runtime** exceptions raised by `cf_dbinfo`, so they are catchable and the preceding page output is emitted. Deliberate divergence: the engine's cfdbinfo attribute validation lives in the tag runtime (matching how the engine handles `<cfheader>`/`<cfhttp>` attribute errors) rather than the compiler; moving it to compile time would be future work. The message text is byte-verified via the `QueryDataTagTest.Dbinfo*` unit tests; the CF-verified .cfm suite cannot cover the abort behavior because CF returns a server-error page.

## `trace()` unknown-attribute errors are runtime; CF rejects them at compile time

CF 2025's `trace(bogus="x")` (and any unknown named argument) is a **compile-time** error — the whole page fails with "Attribute validation error for TRACE tag in cfscript. It does not allow the attribute(s) BOGUS. The valid attribute(s) are ABORT,CATEGORY,INLINE,TEXT,TYPE,VAR.", and a surrounding `<cftry>` cannot intercept it (verified on the RDS host). This engine surfaces the same message from `cf_trace` (src/cffunctions/fn_trace.cpp) at **runtime** as a catchable `Template` error, because the trace() function is a JIT-called runtime built-in with no compile-time named-argument model. Since debugging is disabled (the engine has no debug section) the valid calls all render nothing, so the only observable difference is that a page using an invalid trace() argument renders its preceding output here but not on CF.

Relatedly, a **positional** trace() call (`trace("hello")`, `trace({a:1})`) is rejected by CF 2025 at compile time with "Method trace with N arguments is not in class coldfusion.runtime.CFPage." (uncatchable, like the removed `Location()`). This engine reports `trace("hello")` as the catchable "Attribute validation error for trace." (core_membermethods.cpp) and silently accepts a single positional struct argument (`trace({a:1})` is a no-op) — pre-existing divergences in the function's argument handling, not part of the tag implementation.

## The cache family is sqlite-backed; several CF behaviors cannot be replicated

The `CacheGet`/`CachePut`/`CacheRegion*`/`RemoveCachedQuery` family (implemented 2026-08-08, sqlite-backed via `src/cache_store.cpp` + `src/cffunctions/fn_cache.cpp`) has these deliberate divergences from Adobe CF 2025:

- **Values are stored as SerializeJSON blobs** (like ObjectSave), so a cached complex value round-trips through JSON: queries deserialize to a struct-shaped object unless they were cached by `<cfquery cachedwithin>`/`cacheid` (which use a dedicated type-preserving format via `cf_query_cache_serialize`/`deserialize`). CF stores the live Java object. Simple values (strings/numbers/booleans) and structs/arrays round-trip exactly.
- **`CacheGetSession` throws** — CF returns the underlying Java cache object (a `GenericEhcache`/session handle); this engine has no Java object interop, so like `GetPageContext` it raises `Function CacheGetSession is not supported: it returns a Java cache object.`
- **`cacheGetMetadata`'s TIMESPAN/IDLETIME use resolved seconds** (1 day = 86400s default, matching the decompiled `GenericEhcache.put` which maps a negative interval to the cache config default); CREATEDTIME/LASTHIT/LASTUPDATED are CF DateTime values converted from the stored epoch ms. The SIZE field is the byte length of the stored JSON blob, not ehcache's serialized-size estimate.
- **Query-cache `cacheid` + `RemoveCachedQuery` interplay**: `<cfquery cacheid="x">` stores under the literal id, while `RemoveCachedQuery(sql, dsn, params, region)` computes CF's `29*(29*dsnHash+sqlHash)+paramHash` key, so removing a query that was cached under an explicit `cacheid` needs the same hash path (CF resolves it through its internal `cachedQueryMap`); the primary `cachedwithin` path (hash key) is covered end-to-end.
- **Expiry is lazy** (expired rows are skipped by reads and purged on access), matching the ScopeStore pattern; there is no background eviction thread.

The whole family's byte-verification against CF 2025 is now possible (the `caching` package was installed on the RDS host 2026-08-09). The `<cfcache>` tag is byte-verified (see the PROGRESS.md row); the Cache* functions still diverge from CF and `tests/cfm/tier2_cache_test.cfm` pins the engine's behavior via the CLI with the `CacheTest`/`CfcacheTagTest` unit tests covering the corner cases (see BUGS_CF.md).

## `<cfcache>` divergences

- **`directory` is validated but not used for storage**: CF stores cached pages/fragments in the `directory` attribute's on-disk directory; this engine always uses its SQLite-backed cache store (the `CacheStore`), so the attribute only reproduces CF's validation (empty → "The value of the DIRECTORY attribute is invalid...", non-existent → "The directory (...) specified in the directory attribute in the cfcache tag does not exist.").
- **`username`/`password`/`port` are accepted but functionally ignored** (CF uses them only to build the request URL when building a cache key from a missing request URL); the whole-page cache key is built from the page path + query string instead of the HTTP request URL.
- **Fragment `timespan`/`idletime` defaults**: the engine maps CF's `-1` "not specified" to the cache-config default (1 day), matching the Cache* functions; a literal 0 stays 0 (eternal when both are 0).
- **`action="flush"` with neither `id` nor `expireurl`** removes every TEMPLATE-region entry, matching CF (verified on the RDS host: a whole-page entry disappears after `<cfcache action="flush">`).
- **The whole-page cache hit runs after the Application.cfm prelude** (CF runs the prelude before the page too); `onRequestEnd`/`onError` Application.cfc hooks still run after a whole-page hit (CF's SKIP_PAGE short-circuits only the page body, not the request lifecycle).
- **The FLUSH attribute-combination error** (compile-time) is only reproduced for static `action="flush"` with the observed `id`+`expireurl` and `expireurl`+`throwonerror` conflicts; the JSP TLD combination rules for other static actions are not fully replicated (CF's per-action TLD combos are not accessible — the two generic combos in the message are hardcoded).
- **Empty static `directory`/`dependson` detection covers only plain quoted literals** (`directory=""`); CF also folds constant expressions (e.g. `directory="#trim('   ')#"` is rejected at compile time), which this engine treats as a runtime value (the catchable "The attribute directory specified in the cfcache tag is either empty or invalid.").

## `<cferror>` error-struct divergences (CF 2025)

Byte-verified against the RDS host in tests/cfm/cferror_*_test.cfm except where noted:

- **Diagnostics** is emitted as `{message} {detail}`; CF appends
  ` <br>The error occurred on line {N}.` (this engine's exceptions carry no line
  info).
- **Browser / RemoteAddress / HTTPReferer / QueryString / StackTrace are empty**
  (CF fills them from the request; StackTrace is the real Java stack trace).
  Datetime matches CF's format exactly ("EEE MMM dd HH:mm:ss zzz yyyy").
- **Suppressed is `false`**; CF renders the (always empty) Java array's
  toString, e.g. `[Ljava.lang.Throwable;@3ddb2f31`.
- **The ExceptionScope pseudo-keys (detail, errorcode, extendedinfo, exceptions)
  are stored as real map keys**, so `structKeyList(error)` shows 19 entries
  instead of CF's 15. Reads and structKeyExists behave like CF (verified); only
  key enumeration/structCount differ. The type-dependent pseudo-keys
  (nativeerrorcode, sqlstate, errnumber, missingfilename, lockname,
  lockoperation) correctly stay undefined like CF.
- **`type="validation"` handlers are stored but never dispatched** — this engine
  has no form-validation errors to route to them.
- The request handler's HTTP 500 status cannot be covered by the verify harness
  (it treats HTTP errors as test errors); verified manually (curl showed
  HTTP=500, body byte-identical).

## `<cfloop query group>` validation error leaks the query scope

A `<cfloop query="q" group="badColumn">` with a `group` column the query does
not have throws from `cf_query_group_next` (src/cftags/tag_loop.cpp) — `The
query does not contain a column named 'badColumn' to group on.` — while the
loop's pushed query scope (CF's `pushQueryScope`) is never popped, because the
exception unwinds past the loop's `cf_query_scope_pop` exit block. The leaked
scope stays on `g_queryScopes` for the rest of the request, so a subsequent
`catch` and any later unqualified name that matches a column of the leaked
query resolves against it instead of the variables scope. In the daemon the
leak is contained (scope_begin clears `g_queryScopes` per request); the unit
tests exposed it by running `CfLoopQueryTest.GroupInvalidColumnThrows` before a
component method whose parameter collided with the query's `name` column.
`ComponentTest`'s `SetUp` clears the stack so a leaked scope cannot leak across
tests. Fixing it properly would pop every active query scope on the loop's
exception path (the loop body would need a native EH landing pad), which is
left as future work.

## Web/HTTP/Output tag limitations (cfprocessingdirective, cfsavecontent, cfcookie)

Deliberate divergences in the tags implemented 2026-08-10 (src/cftags/tag_weboutput.cpp + codegen_tags.cpp):

- **`cfprocessingdirective suppressWhitespace` must be a static literal.** CF evaluates the value at runtime (`pushWSManagementSetting`); this engine decides whitespace management at compile time (the WhitespaceState is baked into the emitted code), so `suppressWhitespace="#x#"` is rejected with a compile error instead of being honored like CF.
- **`cfsavecontent`'s invalid-variable-name error type differs.** CF throws `coldfusion.tagext.validation.CFTypeValidatorFactory$InvalidVariableNameException` (a RuntimeException, so `catch(expression)`/`catch(any)` match and `cfcatch.type` shows the full Java class name); this engine throws an `Expression`-typed exception with the same message "Cannot set variable with name X.", so `cfcatch.type` reads `Expression`. Matching (`expression`/`any`) is the same.
- **`cfcookie`'s Set-Cookie byte format is pinned to the RDS host's Tomcat.** The quoted-value/`Version=1`/`Max-Age` formatting and the RFC1123/RFC850 date rendering were derived from CF 2025 on the RDS host; a different CF build/Tomcat cookie processor could serialize cookies differently.

## Input-encoding pipeline: Java vs ICU charset-support sets

The input-encoding order (BOM → `cfprocessingdirective pageEncoding` → ICU
detection → `config::defaultInputCharset`) is byte-verified against CF 2025 in
`tests/cfm/input_encoding_*_test.cfm` + `TemplateEncodingTest` unit tests (see
PROGRESS.md). Two deliberate divergences remain:

- **The supported-charset set is ICU's, not Java's.** CF accepts any charset the
  JVM's `InputStreamReader` supports; this engine accepts any charset ICU4C has
  a converter for (the `pageEncoding` name check and the re-decode both go
  through `ucnv`). The sets overlap heavily (ISO-8859-*, windows-125x, UTF-*,
  Shift_JIS, EUC-*, GB*, Big5, KOI8-*, ...) but names only Java knows (e.g. some
  `x-*`/`IBM*` aliases) throw "The specified page encoding, X, is not supported."
  here.
- **ICU detection results track the ICU build.** The engine links ICU4C 78, whose
  `CharsetDetector` fires at confidence ≥ 100 (the CF default) for a BOM-less
  UTF-16LE source where the RDS host's bundled `com.ibm.icu` (detection disabled
  via `file.usesystemencoding`) does not — see BUGS_CF.md. The default input
  charset and the 100-confidence threshold mirror CF's Administrator settings
  (`config::defaultInputCharset` / `config::charsetDetectionMinConfidence`).

## Text-input vectors do not decode to UTF-8 (raw bytes assumed UTF-8)

The engine's internal strings are UTF-8, and the page-source pipeline already
decodes to UTF-8 on entry (BOM → `pageEncoding` → ICU detection → default
charset, see above). The audit of the *other* text-input vectors found several
that store raw bytes unchanged instead of decoding with the requested charset,
so a non-UTF-8 source yields invalid-UTF-8 strings that corrupt every
downstream string op (`Len`/`Mid`/`ToString`/output encoding):

- **`<cffile action="read" charset="..">` accepts but ignores `charset`.**
  `readFileContents` (src/cftags/tag_file.cpp:144, `(void)charset;`) returns the
  raw bytes; a Latin-1/windows-1252 file lands in the variable as raw bytes
  (invalid UTF-8). The existing tests/cfm/cffile_test.cfm is ASCII-only so this
  is unverified.
- **`FileRead(path, charset)` ignores the charset argument and takes no
  second arg.** The implementation signature is `cf_fileread(path)` only
  (src/cffunctions/fn_fileread.cpp:30); CF's `FileRead(file, charset)` /
  `FileRead(file, bufferSize)` overloads are unimplemented. Raw bytes returned.
- **`FileOpen(path, mode, charset)` accepts but ignores `charset`**
  (src/cffunctions/fn_fileopen.cpp:30); **`FileReadLine`** (fn_filereadline.cpp:30)
  then emits raw bytes, so a non-UTF-8 file opened + read line-by-line yields
  corrupt lines.
- **`<cferror>` handler templates are read as raw bytes** (only a UTF-8 BOM is
  stripped) instead of decoded (src/cftags/tag_cferror.cpp:369) — a non-UTF-8
  error template renders raw/invalid bytes.
- **`<cfinclude>` static (non-CFML) content is appended as raw bytes**
  (src/cftags/tag_include.cpp:69) — same effect for a non-UTF-8 static include.
- **`<cfhttp>` body decoding is partial** (src/cftags/tag_http.cpp:122):
  UTF-8 and ISO-8859-1 are decoded to UTF-8; `windows-1252`/`cp1252` is mapped
  with the Latin-1 rule (wrong for the 0x80–0x9F range: 0x92 should be `'`,
  here it is a C1 control); every other charset (Shift_JIS, EUC-*, GB*, ...)
  falls through to a raw-byte passthrough.
- **FORM/URL/COOKIE scopes decode percent-escapes to raw bytes** and always
  assume UTF-8 (worker.cpp `percent_decode`). This matches CF's default request
  charset, but a query/form body carrying a non-UTF-8 percent-encoded charset
  diverges (CF decodes with the configured request charset).

Write side note: `FileWrite`/`FileWriteLine`/`<cffile action="write"|"append">`
also ignore a requested `charset`, but since internal strings are UTF-8 the
default (UTF-8) output is byte-correct; only an explicit non-UTF-8 write charset
would diverge.

Fix direction (when picked up): expose the ICU decoder already used by the
template reader (`decodeToUtf8`, src/template_reader.cpp, currently in an
anonymous namespace) as a shared `bytes → UTF-8` helper and call it on the
file-read paths; store the charset on the `cfvariant::File` handle so
`FileReadLine` decodes. FORM/URL/COOKIE and `<cfhttp>` use the same helper once
a request charset is honored.

## A SQLite-installed MangoBlog: date functions are ported, but two other queries still failMangoBlog's setup wizard (`admin/setup/setup.cfm`) offers **SQLite** as a
database type and ships `sqlite.sql` (a SQLite port of `mysql.sql`), so choosing
SQLite runs `setupDatabase()`'s `<cfinclude template="sqlite.sql">` and creates
the 19-table blog schema as a `DNS_<name>.sqlite` file. The blog's
`PostsGateway` date queries were then ported from the MySQL/MSSQL
`YEAR()`/`MONTH()`/`DAY()` functions to SQLite's `strftime()` wrapped in
`CAST(... AS INTEGER)` (all 27 usages in `getByDate`/`getCountByDate`/
`getActiveYears`/`getActiveMonths`/`getActiveDays`). SQLite never equates TEXT
with INTEGER, so the CAST is required both for `=` and for `LIKE` on the
zero-padded `%m`/`%d` results (`'08' LIKE 8` is false). The port is deliberate:
`strftime` does not exist on MySQL/MSSQL, which therefore now break on these
queries (accepted tradeoff, the fixture is SQLite-targeted). Guarded by
`CfincludeTest.MangoBlogSqliteSetupSchemaRuns` +
`MangoBlogSqliteStrftimeCastDateQueries`.

**`getActiveYears` / `getActiveMonths` / `getActiveDays` now work on SQLite.**
Three remaining SQLite/engine issues block the rest of the blog:

- **Malformed JOIN chain in `getAll` / `getByDate` / `getByIds`**
  (`PostsGateway.cfc` lines 44 / 226 / 570): `... INNER JOIN post ON entry.id =
  post.id ON post_category.post_id = post.id LEFT OUTER JOIN ...` — the second
  `ON` has no preceding join operator, which SQLite rejects with `near "ON":
  syntax error` (MySQL tolerates the chain). This blocks the blog listing and
  the date-archive post list on SQLite. Fix when picked up: rewrite the FROM to
  valid join syntax, e.g. `category RIGHT OUTER JOIN post_category ON
  category.id = post_category.category_id RIGHT OUTER JOIN post ON
  post_category.post_id = post.id INNER JOIN entry ON entry.id = post.id INNER
  JOIN author ON author.id = entry.author_id LEFT OUTER JOIN entry_custom_field
  ON entry.id = entry_custom_field.entry_id`.
- **`getCountByDate` returns a query column through `returntype="numeric"`**
  and fails "The value returned from the getCountByDate function is not of type
  numeric." This is an engine-wide behavior (not SQLite-specific): a `q.col`
  query-column reference is materialized as an Array-typed lazy cell
  (`cfvariant.cpp` `m_queryColOwner`/`m_queryColIndex`, toString() reads the
  underlying cell), so `IsNumeric(q.col)` is `NO` and typed function returns
  reject it, even when the cell is an integer. Any CFML that passes a raw query
  column into a numeric-typed `returntype`/`cfargument` hits this. Fix when
  picked up: make `IsNumeric`/numeric coercion resolve query-column
  references to their cell value, or materialize `q.col` to the cell's variant
  type instead of a lazy Array ref.
- **`<cflocation>` inside `OnApplicationStart()` does not abort the request.**
  MangoBlog's OnApplicationStart catches `MissingConfigFile` (no config.cfm) and
  does `<cflocation URL="admin/setup/setup.cfm">`; on WebStrada the redirect
  header is emitted but `onRequestStart()` still executes, dying with
  "Element BLOGFACADE is undefined in APPLICATION". Fix when picked up:
  cflocation during any Application.cfc event should terminate further event
  processing for that request (and return the 302).
- **`ExpandPath()` resolves relative paths against the server process CWD
  instead of the base template's directory.** MangoBlog's setup wizard computes
  `path = ExpandPath("../../")` from `admin/setup/setup.cfm`; on WebStrada this
  produced `/home/boris/` (the daemon process CWD chain) instead of the site
  root, writing `config.cfm` outside the webroot so `Mango.init()` never finds
  it. Adobe CF expands against the directory of the currently executing
  template. Related symptom seen earlier in the same flow: the SQLite
  datasource file `DNS_<name>.sqlite` is created under `bin/` (process cwd)
  rather than any deterministic location. Fix when picked up: make ExpandPath
  (and default datasource-file resolution) use the executing template's
  directory / webroot like CF does.
- **`GetDirectoryFromPath()` of a directory path returns the same directory
  instead of its parent.** Adobe CF semantics: `GetDirectoryFromPath("/a/b/")`
  → `/a/`. On WebStrada it returns `/a/b/` unchanged, so chained calls never
  ascend (`d2 == d1`, verified with admin/setup/_pathtest.cfm). MangoBlog's
  setup wizard workaround uses `ListDeleteAt` instead. Fix when picked up:
  strip the last non-empty segment when the input ends with a separator.
- **`<cffile action="write">` (and likely the other file actions) do not
  recognize absolute paths** — an absolute `file="/home/x/y"` is treated as
  CWD-relative and creates `/home/boris/projects/webstrada/home/boris/...`
  (observed writing MangoBlog's config.cfm via PreferencesFile.flush()).
  Workaround in the dev setup: run the daemon with CWD = site root. Fix when
  picked up: detect leading `/` (and drive-letter/UNC forms) and skip CWD
  prefixing in the file tag path resolution.

## Interpreter `#...#` expressions mis-parse member chains with spaces around the dots

Adobe CF accepts whitespace around `.` in member chains everywhere; WebStrada's
JIT paths (`<cfset>`, `<cfif>`, tag attributes) handle it, but the interpreter
path used for `#...#` output expressions does not. The compiled template embeds
the raw expression text (LLVM IR shows a string constant like `"s . a"`) and
hands it to the runtime expression evaluator (`core_interp.cpp`
`applyMemberChain`, which throws "Invalid member access in expression: ." when
a `.` segment trims to empty). Observed with
`<cfset s={a={b={c="w"}}}><cfset k="a">`:

- `#s . a . b . c #` — Adobe prints `w`; we throw `[Expression] Element 'C' is
  undefined` (or emit nothing, depending on surrounding output).
- `#s[k]. b . c #` (space after a bracket hop) — Adobe prints `w`; we print `w`
  followed by a leaked literal `</cfoutput>` (the chain scanner overruns into
  the closing tag text) or throw.
- `#s.a.b.c#` (no spaces) works on both.

Repro/test file: `tests/cfm/member_chain_whitespace_test.cfm` (kept red until
fixed; Adobe reference output is `A:w B:w C:w`). Fix when picked up: make the
runtime expression evaluator's dot-segment scanner skip whitespace around `.`
(the same normalization the JIT `splitMemberPath` helper in
`src/codegen/codegen_expr.cpp` applies), or have codegen normalize spaced
member names before embedding the expression text. Found while writing the
unit test for the chained-member-access fix (2026-08-24).

## `structKeyList(thisTag)` key order differs from CF

ColdFusion's `ThistagScope` stores its keys with the exact casing `hasendtag`,
`executionMode`, `GeneratedContent`, and `StructKeyList(thisTag)` returns them
in that (hash-bucket) order. This engine stores the same three keys with the
same casing but the struct's case-insensitive map orders them
`executionMode,GeneratedContent,hasendtag`, so `structKeyList(thisTag)` —
and any code that iterates the thisTag struct — reports a different order.
The keys, values, read-only behavior (`executionMode`/`hasendtag` reject
assignment), and `generatedContent` semantics are byte-verified against CF
2025 (tests/cfm/custom_tag_forms_test.cfm + custom_tag_thistag_test.cfm);
only the iteration order of the three keys diverges.

## Missing imported-prefix tag is a runtime error; CF rejects it at parse time

CF resolves `<mytag:nonexistent>` (an imported prefix with a missing tag
template) at **parse time** with an uncatchable "Unknown tag: mytag:nonexistent."
error. This engine resolves the tag at **runtime** and throws a catchable
`Unknown tag: nonexistent.` (the taglib directory is stripped from the
reported name because the prefix is not carried into the runtime). The
message text differs slightly and the error is catchable by a surrounding
`<cftry>`, unlike CF. Making it a compile-time error would require resolving
the taglib path against the invoking template's directory during codegen.

## `<cfmodule name="..">` resolves relative to the current template; CF searches the custom-tag library tree

CF's `<cfmodule name="mod">` looks the tag up in the **installed custom-tag
library tree** (server + per-application custom-tag mappings; the `name`
attribute does NOT use the `<cfimport>` prefix mapping — `<cfmodule
name="mytag:mod">` fails with "Cannot find CFML template for custom tag
mytag:mod."). This engine has no custom-tag library tree, so `name` is
resolved as `name.cfm` relative to the invoking template directory. The
`template` attribute (the common form) matches CF exactly.

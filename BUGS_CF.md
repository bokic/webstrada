# Known Bugs and Issues (ColdFusion side)

Bugs, installation problems and behavioral quirks found *inside ColdFusion itself* while
testing against the RDS host (`verify_with_coldfusion.py` / `cfrds`). These are not WebStrada
defects; the engine either cannot be byte-verified against CF for the affected calls or
deliberately reproduces a CF quirk. For bugs in the engine itself see `BUGS.md`; for cosmetic
output artifacts see `BUGS_COSMETIC.md`.

## CF 2021 test server cannot render the `MalformedRegularExpressionException` message

The RDS host at `192.168.100.10` is missing `coldfusion/runtime/StringFunc.MalformedRegularExpressionException.properties`, so any call that throws a malformed-regex error (`REFind("(", ...)`, or ORO-rejected patterns like lookbehind `(?<=...)` / named groups / atomic `(?>` / scoped modifiers `(?i:`) aborts the HTTP response with `server-error: true` (empty body) even inside `cftry/cfcatch` — `#cfcatch.message#` itself throws. Only the `scope` and empty-regex argument errors render (their properties exist). Consequently the regex error-path verification tests (`tests/cfm/refind_re_errors_test.cfm`) cover only those two message shapes; the ORO-rejected constructs are asserted to throw via unit tests (`RegexFunctionsTest.JitRejectsOroUnsupported`) instead. The engine's own messages use ORO's verbatim strings (`Sequence (?<...) not recognized`, etc.).

## Adobe CF 2025 test server (`192.168.100.10`) is missing several standard functions

The `verify_with_coldfusion.py` RDS host reports `server.coldfusion.productName = "ColdFusion Server 2025"`, but `getFunctionList()` does not include `htmlEditFormat`, `getTemplatePath`, or `parameterExists`, and calling them throws `Expression` `Variable X is undefined`. All other functions in `current_progress/` exist on the server. These three therefore cannot be verified against real ColdFusion on this host (affects `HTMLEDITFORMAT.md`, `GETTEMPLATEPATH.md`, `PARAMETEREXISTS.md`). `HTMLEditFormat` was implemented (2026-08-05) by porting `StringFormatter.htmlEditFormat`/`escapeXML(xml=false)`; since the server lacks the function, it is covered by `HtmlEditFunctionsTest` unit tests while the shared escaping is byte-verified through `HTMLCodeFormat` (`tests/cfm/htmlcodeformat_test.cfm`). `GetTemplatePath` and `ParameterExists` are now handled too (2026-08-08, Tier 1): both were **removed** in CF 2025, so the engine dropped them from the built-in registry and reproduces CF's `Variable X is undefined.` for the call form (byte-verified in `tests/cfm/tier1_error_test.cfm`).

Also: `<cftrace var="#struct#">` aborts the request with `server-error: true` even inside `cftry/cfcatch` (debug mode likely disabled), while `<cftrace text="...">` succeeds. Trace's `var` behavior could not be exercised on the server.

## CF 2025 test server: `<cfloop>` directly inside `<cftry>` is a server error

The Adobe CF 2025 server at `192.168.100.10` returns `server-error: true` (response truncated, 0 bytes) for a numeric `<cfloop>` placed directly inside `<cftry>` — verified with `<cftry><cfloop from="1" to="3" index="i"><cfoutput>#i#;</cfoutput></cfloop></cftry>`. The reverse nesting (`<cftry>` inside `<cfloop>`) works, and a cfscript `for` loop inside `<cftry>` works. This is a CF-side nesting limitation, not a WebStrada issue: this engine handles both orders correctly (see `tests/cfm/cftry_tag_test.cfm` case 9, which uses `<cftry>` inside `<cfloop>` to stay verifiable). `<cfrethrow>` outside a `<cfcatch>` and `<cfcatch>`/`<cffinally>` outside a `<cftry>` are rejected at compile time here; CF aborts the page silently.

## CF 2025 test server (`192.168.100.10`) hard-crashes on several image calls

The Adobe CF 2025 server aborts the HTTP response (0 bytes, `server-error: true`) for the following image calls, so they cannot be byte-verified against CF and were excluded from the `tests/cfm/image_*` suite (the engine implements all of them):

- `IsImageFile(path, format)` — the two-argument form always crashes (1-arg form works and matches).
- `ImageWrite(image, destination, "", false)` — an empty-string `quality` argument crashes (numeric quality works).
- `ImageWriteBase64(...)` — crashes in every form tested (`img`, `img,dest`, with/without `format`, `inHTMLFormat`, `overwrite`).
- `ImageGetMetadata(ImageReadBase64(b64))` — metadata on an image read from base64 crashes (file-read images work).
- `FileReadBinary()` of a PNG file crashes (text files are fine) — image base64 round-trips in `tests/cfm/image_readbase64.cfm` therefore go through `ImageGetBlob` instead.

## CF 2025 test server throws `java.util.MissingResourceException` for ImageRead file errors

On `192.168.100.10`, `ImageRead()` of a missing or non-image file throws `[java.util.MissingResourceException] [Can't find resource for base name coldfusion/tagext/io/FileUtils.FileNotFoundException.properties] []` — the server is missing `FileUtils.FileNotFoundException.properties`. This is a broken server install, not the intended Adobe CF message; this engine throws the intended `Expression` `The system cannot find the file specified.` / `The file is not a valid image file.` The ImageRead error-path cases are therefore not byte-compared (see `tests/cfm/image_errors.cfm`, which covers only the ImageWrite errors that render).

## ImageInfo colormodel key order is CF-version-dependent

The `ImageInfo()` `colormodel` struct key order matches the current CF 2025 server byte-for-byte (rgb: `bits_component_3, num_color_components, colorspace, pixel_size, alpha_premultiplied, transparency, alpha_channel_support, bits_component_2, colormodel_type, bits_component_1, num_components`; argb: `colorspace, pixel_size, alpha_channel_support, colormodel_type, num_components, bits_component_4, bits_component_3, num_color_components, alpha_premultiplied, transparency, bits_component_2, bits_component_1`; grayscale: `num_color_components, colorspace, pixel_size, alpha_premultiplied, transparency, alpha_channel_support, colormodel_type, bits_component_1, num_components`). This order is CF's internal struct/hash ordering and is not derived from a documented rule; it may shift across CF versions or server restarts, in which case `src/cfimage.cpp:buildColormodelStruct` needs re-tuning and the byte-exact tests in `tests/cfm/image_new_info.cfm` / `image_readbase64.cfm` / `image_write_roundtrip.cfm` will fail.

Relatedly, the `ImageGetMetadata` JPEG struct's key order also lives in CF's internal HashMap bucket iteration (the engine's capacity-32 uppercase-hash derivation in `buildJpegMetadata`). It **shifted across the 2026-08-10 CF server restart** (feed package install): CF now emits `Image Width` before `Y Resolution` (both hash to bucket 23) where the Aug-7 verification saw `Y Resolution` first. The engine's `src/cffunctions/fn_imagegetmetadata.cpp` was updated to the current order and `image_write_roundtrip.cfm` byte-matches again; if CF restarts again the same swap may need repeating.

## CF 2025 image drawing: return-value stringification NPEs

On the Adobe CF 2025 server, stringifying the return of any `ImageDraw*`/`ImageSet*` function (e.g. `<cfoutput>#r#</cfoutput>` where `r = ImageDrawLine(...)`) aborts the request (0 bytes, `server-error: true`). The functions' documented return type is void; this engine returns a Null variant, and tests never print drawing returns (see `tests/cfm/draw_smoke.cfm`).

## CF 2025 color parsing quirks reproduced in parseDrawColor

- `mediumgray` is listed in CF's own color-list error text but is **rejected** as input (only black/white/gray/darkgray/red/blue/green/pink/orange/magenta/yellow/cyan are accepted). `src/cfimage.cpp:cfColorName` reproduces the rejection; the error text still lists mediumgray.
- A two-part value (`"0,0"`, `"12,34"`) throws `java.lang.StringIndexOutOfBoundsException` `Range [0, -1) out of bounds for length N` where N is the length of the second part — a CF bug, reproduced exactly.
- An empty color string throws `java.lang.StringIndexOutOfBoundsException` `Index 0 out of bounds for length 0`.
- `##FF0000` (double hash) and `#aabb` (short hex) throw the "proper hex format" Application error; single-hash or bare 6-hex work.

## CF 2025 image calls that abort the request even inside cftry/cfcatch

These cannot be byte-verified against CF (curl error 18 / truncated response); this engine instead throws catchable exceptions:

- wrong argument count for an `ImageDraw*`/`ImageSet*` call (e.g. `ImageDrawLine(im,1,1,8)`, `ImageDrawCubicCurve(...10 args)`);
- non-numeric `ImageSetDrawingTransparency(im, "abc")`;
- `ImageDrawLines(im, "1,2,3", "1,2,3")` with comma-string coordinate args;
- `#[` inside `<cfoutput>` (a template-level parse error, e.g. `#sep#[#x#]`).

## CF 2025 ImageSetDrawingStroke: non-width keys alone NPE

`ImageSetDrawingStroke(im, {endcaps:"round"})` (or any key other than `width` without a `width` present) throws `java.lang.NullPointerException` `Cannot invoke "Object.getClass()" because "x" is null` on CF. `src/cfimage.cpp:cf_imagesetdrawingstroke` applies whatever keys are present and does not reproduce the NPE. Negative `width` throws the Application error `The width must be greater than or equal to 0.0.` (reproduced byte-for-byte).

## CF 2025 test server crashes on ALL image text rendering

On the Adobe CF 2025 server, every text-rendering call aborts the request (0 bytes, `server-error: true`) regardless of argument form: `ImageDrawText(im, "Hi", x, y)` (with or without an attributes struct), `ImageCreateCaptcha(...)`, and `<cfimage action="text">` all crash. This looks like a font/JDK issue in the test installation (fonts resolve fine on this host's own JDK). Consequently `ImageDrawText` output **cannot be pixel-verified against CF**. It is implemented best-effort on cairo (`src/cfimage.cpp:cf_imagedrawtext`): baseline at (x,y), `font`/`size`/`style`/`underline`/`strikethrough` attributes, honoring the drawing color, transparency, XOR mode and the drawing-axis transform; font metrics differ from Java2D so glyph shapes/placement are approximate.

## CF 2025 test server crashes on several member-method calls

The Adobe CF 2025 server aborts the HTTP response (0 bytes) for these member-method calls, so they cannot be byte-verified against CF (the engine implements them and they are covered by unit tests instead of the CF-verified suite):

- `arr.sort()` and `arr.reverse()` (array member methods; standalone `arraySort`/`ArrayReverse` render fine on the same server);
- `dt.dateDiff("d", ...)` and `dt.monthAsString()` (date member methods; the standalone `dateDiff` call renders fine);
- printing the return value of a mutating member method in `#...#` (e.g. `#arr.append(4)#`).

## CF 2025 `ImageRotateDrawingAxis` arg-count and type aborts

- `ImageRotateDrawingAxis(im, 45, 5)` (3 args — missing `y`) aborts the request even inside cftry/cfcatch, while both the 2-arg `(im, angle)` and 4-arg `(im, angle, x, y)` forms work. This engine accepts 2-4 args (missing `x`/`y` default to 0).
- `ImageRotateDrawingAxis("notanimage", 45, 0, 0)` also aborts, unlike `ImageTranslateDrawingAxis`/`ImageShearDrawingAxis` which throw a catchable `ClassCastException`.
- `ImageTranslateDrawingAxis` and `ImageShearDrawingAxis` require exactly 3 args; any other count aborts (generic wrong-arg-count abort, see above).

## CF 2025 image ClassCastException message carries an unstable JVM loader hash

Passing a non-image to `ImageTranslateDrawingAxis`/`ImageShearDrawingAxis` throws `class java.lang.String cannot be cast to class coldfusion.image.Image (java.lang.String is in module java.base of loader 'bootstrap'; coldfusion.image.Image is in unnamed module of loader org.apache.felix.framework.BundleWiringImpl$BundleClassLoader @<hash>)`. The trailing `<hash>` is a per-JVM identity hash that changes across restarts, so this engine's `image_from_variant` message (which omits the parenthetical) is not byte-verifiable against CF.

## CF 2025 test server aborts on every `ImageCreateCaptcha` call

The Adobe CF 2025 server at `192.168.100.10` returns `server-error: true` (0 bytes, truncated response) for every `ImageCreateCaptcha` call — valid arguments included — and the exception is not catchable via `cftry/cfcatch` (the request aborts at the server level, likely a headless-graphics/font environment problem: `CaptchaMaker` enumerates `GraphicsEnvironment.getAllFonts()`). Consequently the CAPTCHA output and error messages cannot be byte-verified against CF and the function is excluded from the `tests/cfm/image_*` suite; it is covered by unit tests (`ImageCreateCaptchaDimensions` / `ImageCreateCaptchaErrors` in `tests/tests.cpp`) that port `coldfusion.image.CaptchaMaker` and `ImageHelper.createCaptcha` behavior (auto width/height, difficulty levels, `InvalidCaptchaArgumentException` / `UnsupportedCaptchaDifficultyException` messages).

## CFML `imp` variable name conflicts with the `IMP` operator token

A variable named `imp` renders as empty in `<cfoutput>#imp#</cfoutput>` (and `ImageRead`/`ImageGetWidth` on a variable named `imp` fail with `Variable IMP is undefined`) because the CFML grammar reserves `IMP` as the implication operator; a bare `#imp#` is parsed as the operator keyword and drops out. Other `im*` names (`im`, `ipt`, `img`, `imm`) work. This is a textparser grammar/parser limitation in this engine (reported, not fixed per project rules); the image-metadata cfm tests use `ipm` instead.

## `#CreateODBCDateTime(...)#` interpolated into `<cfquery>` yields `{ts '...'}`, which SQLite rejects (matches CF-on-SQLite)

CFKillBoard's submit SQL originally embedded `#CreateODBCDateTime(KillDateTime)#`, which stringifies to the ODBC literal `{ts '2026-05-21 18:22:00'}`. SQLite rejects that (`unrecognized token: "{"`), and Adobe CF behaves identically on a SQLite datasource — verified on the RDS host (`INSERT ... VALUES (#CreateODBCDateTime("2026.05.21 18:22")#)` fails with `[SQLITE_ERROR] SQL error or missing database (unrecognized token: "{")`). The app is written for SQL Server, which accepts `{ts '...'}`. CFKillBoard's `parser.cfm`/`migrateparser.cfm` were adapted (2026-08-06) to interpolate a quoted ISO literal instead — `<cfset KillDateSQL="#DateFormat(KillDateTime, 'yyyy-mm-dd')# #TimeFormat(KillDateTime, 'HH:nn:ss')#">` then `'#KillDateSQL#'` — so the submit path works on the SQLite backend; the `{ts '...'}` rejection itself is faithful engine behavior (this engine's `<cfquery>` does not translate ODBC escape sequences).

## EXIF metadata key order for images with GPS/Interop/Thumbnail is CF-internal

For images whose EXIF data has more than two directories (IFD0 + SubIFD + GPS/Interop/Thumbnail), the order of keys in `ImageGetEXIFMetadata()`'s struct is driven by com.drew's `Metadata` map, which iterates directories by `HashMap<Class>` order (Java class identity hashes) and the tags within each directory by their own map order — both non-deterministic across CF restarts (the `Class` identity hash changes per JVM). This engine emits a deterministic order (IFD0 → SubIFD → GPS → Interop → Thumbnail, tags ascending) whose values are byte-identical to CF but whose key order does not always match a given CF instance. The single/multi-directory cases that ARE stable (IFD0-only, IFD0+SubIFD, IPTC) are byte-verified in `tests/cfm/image_exif_metadata.cfm`; GPS tag *values* are verified via the order-independent `ImageGetEXIFTag` getters in the same file + `ImageExifGpsTags` unit test.

## CF 2025 `<cfimage>` tag: several actions/errors abort the page

The Adobe CF 2025 server at `192.168.100.10` aborts the HTTP response (0 bytes, `server-error: true`) for several `<cfimage>` cases, so they cannot be byte-verified and are excluded from the `tests/cfm/cfimage_tag_*.cfm` suite (the engine implements all of them with CF's property-file messages, covered by `ImageCfimageTag*` unit tests in `tests/tests.cpp`):

- `action="captcha"` in any form (same headless font/graphics failure as `ImageCreateCaptcha` — see above).
- `action="bogus"` (UnsupportedImageActionException) and `thickness="-1"` (NegativeThicknessException) abort even inside `cftry/cfcatch`, unlike the empty-`destination` / empty-`name` errors (InvalidDestinationException / InvalidNameException) which ARE catchable and byte-verified.
- `<cfimage action="read" ...>` with no `name` and `<cfimage action="write" ...>` with no `destination` abort (the engine follows the decompiled `ImageTag` logic: read/write with no destination simply skip the write and still create `name` when given).
- `<cfimage>` with `source="#ImageNew(...)#"` (an inline function call in the attribute) aborts — the engine evaluates it fine; tests use a pre-created variable.

## CF 2025 `<cfimage>` writetobrowser / inline-captcha `<img>` names are random

`writetobrowser` and `action="captcha"` with neither `name` nor `destination` write a temp file named `_cfimg<random>.<format>` / `_captcha_img<random>.png` under `CFFileServlet/_cf_image` / `CFFileServlet/_cf_captcha` and emit `<img src="/CFFileServlet/_cf_image/..." ... />`. The random file name cannot be byte-compared (it changes per request), so the tag *structure* is asserted in unit tests; the engine's temp files go under the OS temp directory with the same naming/URI.

## CF 2025 test server compiles every `<cfinclude>` target as CFML regardless of extension

The Adobe CF 2025 server at `192.168.100.10` has `compileExtForInclude` set to a wildcard (CF Admin → Server Settings → "compile extensions for include"), so a `<cfinclude>` of a non-CFML file (`.txt`, `.xyz`, ...) is compiled and executed as CFML instead of being included as static content. Verified: including a file containing `raw #foo# <cfset z=7>` executes the `<cfset>` (a later `#z#` reads `7`) and strips the tag, whereas a default CF install (empty `compileExtForInclude`) reads such files verbatim. This blocks byte-verification of the static-include path (CF's `IncludeTag.checkForType` / `addStaticTemplateContent`): the engine follows the default-CF behavior (only `.cfm`/`.cfml` targets are compiled; any other extension is read and output raw, no CFML evaluation). The `.txt` include test (`tests/cfm/cfinclude_static_test.cfm`) therefore uses content that renders identically whether compiled or static; the static-vs-compiled distinction is covered by the `CfincludeTest.StaticNonCfmlFileOutputRaw` unit test.

## CF 2025 whole-column assignment with an array RHS is erratic (moved from BUGS.md #5)

Scalar whole-column assignment (`q.a = v` writing the current row's cell, `q.zzz = v` → `Application: There is a problem in the column mappings...`, bracket `q["a"] = v` → `Expression: An error occurred while trying to modify the query named class coldfusion.sql.QueryTable.`) is implemented and byte-verified against CF 2021/2025 (`tests/cfm/query_column_assign_test.cfm`). The remaining case — an **array** (or other complex value) RHS `q.a = [..]` — is a CF-side erratic behavior, deliberately not replicated: CF's own results vary per run (a 2-element array on a 2-row varchar query was silently accepted in one probe; a 3-element array on a 3-row query threw `Expression:` with an empty message; a variable-held array threw in yet another layout with an empty message after aborting the page in one run). The engine rejects it with the same `Expression: An error occurred while trying to modify the query...` message as the bracket case.

## CF 2025 bare `throw` with a literal-first arithmetic operand is erratic (moved from BUGS.md #7)

Adobe CF 2025's script bare-throw grammar reads the first operand only when it is a literal, and its behavior is inconsistent across operand forms: `throw 5+3` throws message `5` (the `+3` is dropped), but `throw "msg" & "x"` and `throw 5 * 2` crash the server entirely (0-byte response), while `throw x + 1` (variable operand) evaluates the full expression (`43`). There is no stable, consistent rule to replicate — the engine deliberately evaluates the whole bare-throw expression (`throw 5+3` → message `8`), which is the natural reading. Documented, not silently wrong.

## CF 2025 `rethrow;` outside any catch silently aborts (moved from BUGS.md #7)

Adobe CF 2025 silently aborts the request (empty page, no error, no server-error header) for a bare `rethrow;` at template top level (no enclosing catch). This engine rejects it at compile time with `'rethrow' is only valid inside a catch block` — a deliberate model choice (the Java/C++ equivalent of rethrowing outside a handler) that surfaces the programming error instead of silently halting; the user-visible behavior differs from CF.

## CF 2025 crashes on `<cfflush>` inside `<cfsilent>` (moved from BUGS.md #10)

The Adobe CF 2025 server aborts the HTTP response with a large error page (not catchable via `cftry/cfcatch`) for `<cfoutput>A|</cfoutput><cfsilent><cfoutput>X</cfoutput><cfflush><cfoutput>Y</cfoutput></cfsilent><cfoutput>|B</cfoutput>` — the flush inside the silent body crashes the server, so the intended "commit and drop everything after" behavior cannot be byte-verified. This engine's `cf_response_flush` refuses to flush a silent buffer, producing `A||B` (the suppressed `X`/`Y` never leak, and `|B` is not dropped because the response was never committed).

## CF 2025 OLE date arithmetic is off by 2 days before the epoch (year < 1900)

`DateDiff("d", CreateDateTime(1899,12,30,0,0,0), <date>)` on the RDS host returns values 2 days larger in magnitude than the true proleptic-Gregorian serial for dates before the OLE epoch: e.g. `LSParseDateTime("15.05.20","English (US)")` yields year 20 AD, and CF's `DateDiff("d", 1899-12-30, that)` = -686521 while the correct Gregorian serial is -686519 (verified with Python's `datetime`). The 2-day gap is the OLE automation date system's known pre-1900 discrepancy (the phantom 1900-02-29 + the 1899/1900 leap handling). This engine's `tmToDays` produces the correct Gregorian serial (-686519), so `DateDiff`/serial comparisons against CF diverge only for dates before 1899-12-31. Same class of quirk: CF's `{ts '...'}` serialization century-pads years < 100 (a year-20 date prints as `{ts '2020-05-15 ...'}` — see BUGS_COSMETIC.md); the engine prints the true year.

## CF 2025 silently aborts the page on `return`/`<cfreturn>` outside a function (moved from BUGS.md)

Adobe CF 2025 does NOT reject `<cfscript>return 5;</cfscript>` at template top level or a tag-form `<cfreturn>` outside a `<cffunction>` — it silently stops the rest of the page (no error, no server-error header, HTTP 200), aborting the template like a bare `cfabort` (verified on the RDS host: output before the statement is kept, everything after is dropped). This engine deliberately rejects both at compile time instead — `'return' is only valid inside a function` / `'cfreturn' is only valid inside a cffunction` — because silently halting the request on a misplaced `return` hides a programming error (the CFML equivalent of a Java/C++ `return` outside a method). Kept as a deliberate model divergence, documented here rather than in BUGS.md.

## CF 2025 test server lacks the pmtagent monitoring package

`GetSystemFreeMemory()` and `GetSystemTotalMemory()` on the RDS host throw `The pmtagent package is not installed.` (the OS/JVM monitoring agent is not installed in the test server), so the byte values cannot be verified against CF 2025. This engine implements the documented semantics with `sysinfo(2)` (free/total OS memory in bytes); the return type/positivity are asserted in `JitExpressionTest.Tier1SystemMemory`.

## CF 2025 test server has no PDF/DDX service, so IsDDX always reports NO

`CFPage.IsDDX` delegates to `ServiceFactory.getPDFService().validateDDX(...)` and returns `false` when the PDF service is null; the RDS host has no PDF extension, so `IsDDX('<DDX>...</DDX>')` reports NO for every input (even a well-formed DDX packet). The engine's sniffing implementation (parse XML, case-sensitive `<DDX>` root) follows the documented behavior but cannot be byte-verified against this host (see `JitExpressionTest.Tier1IsDdxAndWddx`).

## CF 2025 `DeleteClientVariable` returns YES whenever the client scope exists

On the RDS host client storage is enabled, so the `client` scope exists and CF's `DeleteClientVariable` (OtherFunc) returns `true` for **any** name — even a non-existent one — because only the removal is conditional, not the return. The engine (which has no client scope) reproduces this host's `YES` for the common case, byte-verified in `tests/cfm/tier1_basic_test.cfm`. On a CF server with client storage disabled the function would return `false` instead.

## CF 2025 removed the script `Location()` function

`Location(url[, addtoken][, statuscode])` no longer exists in CF 2025 (it was a script alias for `<cflocation>` through CF 2021): calling it in any arity throws `Method Location with N arguments is not in class coldfusion.runtime.CFPage.` and the error is NOT catchable by `cftry/cfcatch` (the whole page aborts). `getFunctionList()` still lists LOCATION. The engine reproduces the method-not-found message with the actual argument count (`JitExpressionTest.Tier1LocationThrowsLikeCf2025`); the `<cflocation>` tag keeps working.

## CF 2025 removed `CreateGUID`; `CreateUUID` uses an 8-4-4-16 shape

On the Adobe CF 2025 RDS host (`192.168.100.10`) `CreateGUID()` throws `Variable CREATEGUID is undefined.` — the function was removed in CF 2025 (its script form is gone). The engine kept a `CreateGUID` alias for older-CF compatibility, which is a deliberate divergence (the alias is documented in PROGRESS.md). Also, CF's `CreateUUID()` returns an **8-4-4-16** string (35 chars: 32 hex digits + 3 dashes, e.g. `C6B34173-E479-FF88-D83FCE70D36BCF12`), NOT the standard 8-4-4-4-12 UUID (36 chars) — the engine's `IsValid("uuid")` validator and `CreateUUID()` now match this CF shape byte-for-byte (the CF UUID regex in `CFTypeValidatorFactory` is `8-4-4-16`).

## CF 2025 `caching` package: installed 2026-08-09 — Cache* function divergences now observable

- **`<cfcache>` is byte-verified** (tests/cfm/cfcache_test.cfm + tier2_cfcache_test.cfm, both 🟢): clientcache rendering/headers, catchable validations, object put/get/flush, metadata, fragment hit behavior, stripwhitespace (`\r` replacement), dependsOn, and the get-miss leaving the name variable undefined.
- **The Cache* functions still diverge** (tests/cfm/tier2_cache_test.cfm fails): CF's `cacheGetProperties` returns the region's full ehcache configuration (NAME, OBJECTTYPE, MAXBYTESLOCALHEAP, TIMETOLIVESECONDS, ... and ~35 more keys), not just the custom properties; `cacheSetProperties({foo:"bar"})` ignores unknown property names instead of storing them; and other Cache* function details (metadata keys, error messages) are only pinned by the engine's `CacheTest` unit tests, not yet byte-verified. Fixing these is a separate task from the `<cfcache>` tag.

## CF 2025 RDS host: `zip` package installed 2026-08-09 — cfzip now byte-verified

The Adobe CF 2025 RDS host at `192.168.100.10` previously lacked the CF `zip` package, so every `<cfzip>` call threw `The zip package is not installed.`. The package has since been installed (`/opt/coldfusion/cfusion/bin/cfpm.sh install zip`, server restarted), unblocking cfzip byte-verification. The engine's `<cfzip>`/`<cfzipparam>` (added 2026-08-09, minizip-backed) is now byte-verified against CF 2025 in `tests/cfm/tier2_cfzip_test.cfm` (🟢): zip of a directory source (entries relative to the source, directory entries with a trailing `/`), the 9-column list query (NAME is the full entry path, DIRECTORY the parent, plus SIZE/COMPRESSEDSIZE/TYPE/DATELASTMODIFIED/COMMENT/CRC/ENCRYPTIONALGORITHM), read/readBinary, unzip (destination must already exist — CF's InvalidDestinationException otherwise), delete, cfzipparam content/source, the action-specific compile-time attribute/required-attribute validation, and the catchable Application error messages (missing entry, missing zip file). Note: the raw list entry order is archive-iteration dependent (zip4j on CF vs minizip here), so the cfm test sorts the collected names; `dateLastModified` is creation-time dependent and is not byte-compared.

## CF 2025 `<cfstoredproc returncode="yes">` on MySQL errors; the engine returns statuscode 0

Once a MySQL datasource (`mysqltest`) was registered on the RDS host, `tests/cfm/cfstoredproc_test.cfm` runs on CF and its `returncode="yes"` section throws `Error Executing Database Query. Parameter number 4 is not an OUT parameter` (Connector/J against a 3-parameter `IN a, IN b, OUT s` procedure): CF's `StoredProcTag` emits the ODBC escape `{? = call proc(?,?,?)}` and registers a 4th OUT for the status code, which MySQL procedures cannot satisfy. The engine does not reproduce this CF-on-MySQL quirk — it executes a plain `CALL` and reports `statuscode` 0 (MySQL procedures have no return value), so its `cfstoredproc` output (`A[5|false|YES] B[15|0] C[3|1|name1]`) cannot be byte-compared on the returncode section. The non-returncode sections byte-match when run standalone. This is a CF-side limitation (MySQL + returncode), not an engine divergence; kept as a documented divergence in PROGRESS.md's cfstoredproc row.

## CF 2025 `<cfparam>` exception-message cache can show a stale value

While probing `<cfparam>` type-validation errors on the RDS host, running several
`<cfparam type="integer" default="#invalid#">` probes back-to-back inside a
cffunction (reusing one variable) produced the **previous** probe's value in the
`IntegerParseException` message — e.g. `default="1.5"` reported
"The value specified, abc, must be a valid integer." where `abc` was an earlier
probe's default. Standalone probes render the correct value. The cause is CF's
`CFTypeValidationException.getMessage()` caching `this.message` on the exception
instance while the JSP tag framework reuses/pools instances, so a stale message
survives into the next probe's `cfcatch.detail`. The engine always renders the
correct value (verified directly); only CF's pooled-message quirk was affected,
so `cfparam` probes were repeated standalone to keep byte-verification clean.

Relatedly, CF's `<cfobjectcache>` REQUIRES the `action` attribute at compile time
("Attribute validation error for the CFOBJECTCACHE tag. The tag requires the
ACTION attribute.") despite the cfml-reference doc listing a `clear` default —
an early probe that put `<cfobjectcache>` (no attributes) after a static-bad-action
tag looked like a success but was actually aborted by the earlier compile error.

## CF 2025 RDS host: `feed` package installed 2026-08-10 — cffeed now byte-verified

The Adobe CF 2025 RDS host at `192.168.100.10` previously lacked the CF `feed`
package, so every `<cffeed>` call threw `The feed package is not installed.`.
The package has since been installed (the `feed-2025.0.0.331385.jar` bundle in
the installer payload is now loaded), unblocking cffeed byte-verification. The
engine's `<cffeed>` (added 2026-08-10) is now byte-verified against CF 2025 in
`tests/cfm/cffeed_test.cfm` (🟢): RSS 2.0 read (feed struct values, item struct
with description {type,value}/guid{value,isPermaLink}/category arrays, the
FeedTable query columns), Atom 1.0 read (title/summary/content Content structs,
author arrays, query), and RSS 2.0 / Atom 1.0 create (Rome's CRLF pretty-printed
XML, RFC822/W3C date rendering, the channel/item and feed/entry element order).

## CF 2025 RDS host: `<cfexecute>` cannot capture output without a timeout

On the RDS host the JVM's `ProcessExecutor` reader threads never finish before
`getData()` is read when no `timeout` attribute is given (ProcessExecutor does
not wait at all in that case — `blocking` is false and `timeout` is 0), so the
output is consistently NOT captured: the `variable` stays undefined and the page
gets nothing. With `timeout="N"` the join waits and the output is captured. The
engine reproduces this race faithfully (cf_execute_tag returns without binding
the variable when no timeout is given) and byte-verifies the with-timeout path,
the timeout error message, the exec-failure message and the page-output case in
`tests/cfm/cfexecute_test.cfm` (🟢).

## CF 2025 RDS host: ICU charset detection is effectively disabled

The RDS host's ColdFusion reads every template that has neither a BOM nor a
`cfprocessingdirective pageEncoding` directive with the JVM default charset
(UTF-8): a windows-1252 file with a smart-quote byte (0x93) renders U+FFFD, and a
UTF-16LE file without a BOM is served as its raw bytes — i.e. the `TemplateReader`
ICU `CharsetDetector` path never fires. This is consistent with the host running
with `-Dfile.usesystemencoding=true` (which nulls the detected encoding) or with
the bundled `com.ibm.icu` never reaching the 100-confidence threshold. WebStrada
implements CF's shipped design (ICU detection with the default 100-confidence
threshold from `coldfusion.charsetdetection.minthreshhold`, `config::
charsetDetectionMinConfidence`), so ICU4C 78 detects a BOM-less UTF-16LE source
(confidence 100) where the RDS host does not. The detection-dependent fixture was
therefore dropped from the byte-verified `input_encoding_*` suite and is pinned
by the `TemplateEncodingTest.Utf16WithoutBomDetectedByIcu` unit test instead.

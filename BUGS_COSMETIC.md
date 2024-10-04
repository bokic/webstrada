# Known Bugs and Issues (cosmetic, this codebase)

Cosmetic output/display artifacts in WebStrada — whitespace, dump formatting and other
presentation-only differences from ColdFusion that do not affect functional correctness.
These are tracked separately from real engine bugs (`BUGS.md`) and from ColdFusion-side
issues (`BUGS_CF.md`).

## Leading space before a `<style>` block after a multi-line `<cfoutput>`

A `<cfdump>` following a multi-line `<cfoutput>` (a `\n` before `<cfdump>`) emits ` <style>` while CF emits `<style>`. Reproduces with a struct too, so it is not query-specific — a pre-existing cfdump/page whitespace artifact.

## `<cfset>` variable names keep their original casing in cfdump

`<cfset myvar = 1><cfdump var="#variables#">` shows the key `myvar` (original casing) while CF 2021 shows `MYVAR`. Struct-literal keys and UDF names are already uppercased consistently; only cfset (and similar variable-binding) keys diverge. Pre-existing; unrelated to the UDF dump work. Reproduces in the HTML and text dumps of the whole `variables` scope.

## A `<cfscript>` sticky whitespace flag inserts a space between consecutive text-format `<cfdump>`s

`<cfscript>...</cfscript>` followed by two `<cfdump var="#x#" format="text">` tags emits `</pre> <pre>` while CF emits `</pre><pre>`. Reproduces without UDFs (verified on the pre-fix tree), so it is a pre-existing whitespace-management artifact of the compiler, not a cfdump rendering difference.

## cfdump text dump emits one extra trailing newline (pre-existing, `--exact` only)

`tests/cfm/cfdump_udf_test.cfm` fails the strict `--exact` byte comparison: the trailing `</pre>` is followed by an extra `\n` in our output that CF 2021 does not emit. The non-exact comparison (default) passes. This is a pre-existing cfdump/page-trailing-whitespace artifact, unrelated to the `<cfcontent>`/`<cfflush>` work (reproduces on the pre-change tree). Compare `cf_output == our_output` in `verify_with_coldfusion.py`.

## A `<cfscript>` sticky whitespace flag inserts a space after a UDF's `writeOutput` before a following `<cfoutput>`

When a `<cfscript>` block's output comes from a UDF call (`<cfscript>function foo(){writeOutput("X");} foo();</cfscript>` then `\n<cfoutput>\nA|</cfoutput>`), this engine emits `X| \nA|` (a space after `X|`) while CF 2025 emits `X|\nA|`. Direct `writeOutput("X");` in the same script (no UDF) matches CF (`X Y`). Reproduces with plain `writeOutput` in the UDF (no IsDefined involved), so it is a pre-existing whitespace-management artifact of the compiler, not an IsDefined difference. `tests/cfm/isdefined_test.cfm` avoids the pattern by returning the UDF's output as a string rendered through `<cfoutput>`.

## Boolean literal keywords do not preserve their original case

CF preserves the exact spelling of a boolean literal keyword when stringified: `<cfset b=TRUE>#b#` → `TRUE`, `True` → `True`, `false` → `false` (verified on the RDS host, CF 2025). This engine normalizes every literal to lowercase `true`/`false`, so `<cfset b=TRUE>` renders `true`. The literal-vs-computed distinction (`#true#` → `true`, `#(5 GT 3)#` → `YES`) already matches CF (tests/cfm/bool_stringification_test.cfm); only the *case* of the literal keyword is lost. The boolean *value* is identical either way — a presentation/round-trip divergence, not a logic one.

## Cairo vs Java2D AA-off rasterization for strokes (≤1 px, moved from BUGS.md #9)

WebStrada renders through cairo (chosen coarse/approximate for this feature). Pixel-exact primitives (verified against CF): ImageClearRect, ImageDrawRect (fill + outline), ImageDrawPoint, ImageDrawBeveledRect (both tone layouts), axis-aligned ImageDrawLine, ImageDrawLines filled polygons, XOR, and transparency blends. Stroked curves/ovals/arcs/roundrects and diagonal lines match CF's bounding box within ±1 px and use identical colors, but individual pixels differ at shape extremes/endpoints (Java2D turns on partially-covered pixels where cairo stops at the exact coordinate). The same ±1 px shift applies to shapes drawn under a non-translational drawing-axis transform (e.g. `ImageRotateDrawingAxis(im, 90, 15, 15)` strokes land one column/row off CF); pure `ImageTranslateDrawingAxis` cases are pixel-exact. Also, cairo transforms the stroke *width* with the matrix (a 1 px stroke under a 45° rotation is ~1.41 px), whereas Java2D strokes in device space; this diverges only for arbitrary-angle rotations/shears and is accepted under the coarse comparison — a ≤1 px presentation difference, not a functional one.

## CF's `{ts '...'}` date serialization century-pads years < 100 (display only)

Outputting a date whose year is 1–99 directly (`<cfoutput>#d#</cfoutput>`) on CF 2025 renders the year century-padded: `LSParseDateTime("15.05.20","English (US)")` parses to year 20 AD (confirmed via `DateFormat(d,"yyyy")` → `0020` and `DateDiff`), yet CF prints `{ts '2020-05-15 00:00:00'}` — the same string as for year 2020. This engine's `{ts '...'}` serializer shows the true year (`{ts '0020-05-15 00:00:00'}`), matching what `DateFormat`/`LSDateFormat` report; the underlying OLE value is identical. Byte-for-byte tests therefore avoid direct `#d#` output for dates with year < 100 (they use `DateFormat`/`TimeFormat` masks, e.g. `tests/cfm/lsparsedatetime_locale_test.cfm`). A presentation divergence, not a value one.

## `GetFunctionList()` enumerates the engine's registry, not CF's Java-method list

CF builds the map from `CFPage.getMethods()` (mixed-case Java method names, 800 entries); this engine enumerates `kBuiltinFunctionNames` (uppercase, 657 entries), so the key set/casing and `StructCount` differ even though every value is the empty string like CF (byte-verified in `tests/cfm/tier1_getfunctionlist_test.cfm` for the memberships that agree). `GetMetricData("perf_monitor")` returns CF's exact key set but idle zero values, since the live server counters differ per host. Both are presentation/state differences, not functional ones.

## A `#...#` expression error mid-`<cfoutput>` loses the space before the catch output

When a `<cfoutput>` body is interrupted by an exception inside an interpolation (`<cfoutput>before#1 / 0#after</cfoutput>` then `<cfcatch>`), CF emits a space between the output text and the following `<cfoutput>` in the catch (`before CAUGHT`); this engine emits `beforeCAUGHT`. A `<cfoutput>` whose whole body is a single throwing `#...#` matches CF exactly, so the difference is a whitespace-management artifact of the interrupted-interpolation path, not the stacktrace/TAGCONTEXT work (reproduces on the pre-stacktrace tree). Caught output compared non-exact (stripped) in `verify_with_coldfusion.py`.

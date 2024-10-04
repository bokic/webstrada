# Research: HTMLEditFormat cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Real typed implementation exists: `cfml::cf_htmleditformat(const cfvariant *, const cfvariant *)` in `src/cf8.cpp:16408` (ports `StringFormatter.escapeXML(xml=false, escapeNewSet=false)`; the optional `version` argument is ignored).
- Compiler: native JIT handler at `src/compiler.cpp:2896`; symbol registered with the correct two-arg signature at `src/compiler.cpp:7223`.
- Interpreter (`evaluateExpr`) dispatch at `src/cf8.cpp:6923`.
- Runtime dispatch (`cfvariant_call_function`) case present (dynamic-call paths).

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Real typed implementation | `src/cf8.cpp:16408` |
| Compiler wiring | ✅ Native JIT handler (1-2 args) | `src/compiler.cpp:2896`, symbol at `src/compiler.cpp:7223` |
| Interpreter dispatch | ✅ Present | `src/cf8.cpp:6923` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `HtmlEditFunctionsTest` unit tests (JIT + interpreter paths); NOT byte-verified against the RDS host (the CF 2025 server lacks `htmlEditFormat` — see `BUGS_CF.md`); the shared escaping is byte-verified via `tests/cfm/htmlcodeformat_test.cfm` | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_HTMLEDITFORMAT.md` |

## What HTMLEditFormat does at the low C level

Per Adobe CF (`StringFormatter.htmlEditFormat` → `escapeXML(html, out, false, false)`): replace special characters in the input string with their HTML-escaped equivalents, no `<pre>` wrapper. Note: earlier research notes claiming `CR → <br>` were wrong — CF **drops** `\r` (case `'\r': break`); `\n` is preserved.

Escaping (same as HTMLCodeFormat, verified via the shared code in the byte-verified HTMLCodeFormat test):
- `\r` is dropped.
- `"` → `&quot;`
- `&` → `&amp;`
- `'` → `'` (unchanged)
- `<` → `&lt;`
- `>` → `&gt;`
- `\n`, tabs, `%` and all other characters pass through unchanged.

## Parameter passing (by value / by reference)

- Arg 1: `String` — by value (converted via `variantToString`).
- Arg 2: `version` — optional, ignored; must be present to compile but has no effect.

## CF server note

`htmlEditFormat` is missing from the RDS host (`getFunctionList()` does not include it and calling it throws `Variable HTMLEDITFORMAT is undefined`, aborting the page), so this function cannot be byte-verified there. The implementation is ported from the decompiled `StringFormatter` and the escaping is byte-verified through `HTMLCodeFormat`.

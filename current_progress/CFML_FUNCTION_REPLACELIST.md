# Research: ReplaceList cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Real typed implementation exists: `cfml::cf_replacelist(const cfvariant *, const cfvariant *, const cfvariant *, const cfvariant *, const cfvariant *, const cfvariant *)` in `src/cf8.cpp:19088` (ports `StringFunc.ReplaceList` + `CFPage.ReplaceList` argument handling).
- Compiler: native JIT handler at `src/compiler.cpp:2920`; symbol registered with the correct six-arg signature at `src/compiler.cpp:7418`.
- Interpreter (`evaluateExpr`) dispatch at `src/cf8.cpp:6945`.
- Runtime dispatch (`cfvariant_call_function`) case present (dynamic-call paths).

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Real typed implementation | `src/cf8.cpp:19088` |
| Compiler wiring | ✅ Native JIT handler (3-6 args) | `src/compiler.cpp:2920`, symbol at `src/compiler.cpp:7418` |
| Interpreter dispatch | ✅ Present | `src/cf8.cpp:6945` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/replacelist_test.cfm` verified byte-for-byte against CF 2025 (interpreter + cfscript JIT paths) + `HtmlEditFunctionsTest` unit tests | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_REPLACELIST.md` |

## What ReplaceList does at the low C level

Per Adobe CF: replace occurrences of elements from `list1` in the input string with the corresponding elements from `list2` (by position), applied **pair by pair, sequentially, replacing ALL occurrences** of each search token, case-sensitively. Default delimiter is comma.

Argument handling (ports `CFPage.ReplaceList`):
- 3 args: delimiters `,`/`,`, includeEmptyFields=false.
- 4 args: a `true`/`yes` value → includeEmptyFields=true with comma delimiters; `false`/`no` → includeEmptyFields=false; any other string is used as both delimiters.
- 5 args: a `true`/`yes` 5th arg → includeEmptyFields=true with the 4th arg as both delimiters; `false`/`no` → includeEmptyFields=false; otherwise the 4th/5th args are the two delimiters.
- 6 args: 4th/5th delimiters + boolean includeEmptyFields.

`StringFunc.ReplaceList` core loop (empty list1 tokens skipped; a shorter list2 makes the surplus pairs replace with `""`; empty list2 values are skipped by advancing list2 only while includeEmptyFields is false). Delimiters follow `ListFunc.escapeDelim(delim, false)`: a multi-char delimiter is a character set, each regex metachar is escaped (so `.`/`|` delimiters do not split a comma list), and an empty delimiter splits the string into its individual characters (Java `split("")` semantics) — all byte-verified against CF 2025.

## Parameter passing (by value / by reference)

- Args 1-3: `String` — by value (converted via `variantToString`).
- Args 4-6: optional delimiters / includeEmptyFields.

## CF server note

`&#169;`/`&#x41;` (`#` inside a `#...#` sharp expression) aborts the request on both engines (nested sharp-expression start); the byte-verified tests avoid such input.

# Research: HTMLCodeFormat cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Real typed implementation exists: `cfml::cf_htmlcodeformat(const cfvariant *, const cfvariant *)` in `src/cf8.cpp:16398` (ports `StringFormatter.escapeXML(xml=false, escapeNewSet=false)`; the optional `version` argument is ignored).
- Compiler: native JIT handler at `src/compiler.cpp:2896`; symbol registered with the correct two-arg signature at `src/compiler.cpp:7222`.
- Interpreter (`evaluateExpr`) dispatch at `src/cf8.cpp:6934`.
- Runtime dispatch (`cfvariant_call_function`) case present (dynamic-call paths).

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Real typed implementation | `src/cf8.cpp:16398` |
| Compiler wiring | ✅ Native JIT handler (1-2 args) | `src/compiler.cpp:2896`, symbol at `src/compiler.cpp:7222` |
| Interpreter dispatch | ✅ Present | `src/cf8.cpp:6934` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/htmlcodeformat_test.cfm` verified byte-for-byte against CF 2025 (interpreter + cfscript JIT paths) + `HtmlEditFunctionsTest` unit tests | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_HTMLCODEFORMAT.md` |

## What HTMLCodeFormat does at the low C level

Per Adobe CF: HTML-escape special characters in the input string and wrap the result in `<PRE>`/`</PRE>` tags (uppercase, unlike the `cfml_docs` lowercase example). The only difference from `HTMLEditFormat` is the wrapper.

Escaping (verified byte-for-byte against CF 2025):
- `\r` is dropped (case `'\r': break`).
- `"` → `&quot;`
- `&` → `&amp;`
- `'` → `'` (unchanged — `HtmlAssembler.SINGLEQUOTE` is a literal single quote)
- `<` → `&lt;`
- `>` → `&gt;`
- `\n`, tabs, `%` and all other characters pass through unchanged.

## Parameter passing (by value / by reference)

- Arg 1: `String` — by value (converted via `variantToString`).
- Arg 2: `version` — optional, ignored; must be present to compile but has no effect.

## CF server note

On the RDS host at `192.168.100.10`, a string containing `&#169;`/`&#x41;` (a `#` inside `#...#`) aborts the request both here and on CF (`#` inside a sharp expression starts a nested expression); use `&copy;`-style named entities instead. See `BUGS_CF.md`.

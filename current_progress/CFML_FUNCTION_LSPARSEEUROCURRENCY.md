# Research: LSParseEuroCurrency cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_lsparseeurocurrency()` in `src/cf8.cpp:14780` throws `"Function LSParseEuroCurrency is not implemented"`.
- Compiler: `LSPARSEEUROCURRENCY` is in the zero-arg not-implemented function list (`src/compiler.cpp:1742`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5545`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14780` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1742`, symbol at `src/compiler.cpp:5545` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*lsparseeurocurrency*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:624` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_LSPARSEEUROCURRENCY.md` |

## What LSParseEuroCurrency does at the low C level

Per Adobe CF: parse a locale-specific currency string as a number, trying each default currency format (`none`, `local`, `international`) in turn. Ensures correct euro handling for Euro-zone locales.

- Arg 1: `string` (input).
- Arg 2: `locale` (optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `string` (input). passed by value.
- Arg 2: `locale` (optional). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_lsparseeurocurrency(const cfvariant *str, const cfvariant *locale);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- Full typed implementation using the existing locale infrastructure; nearly identical to `LSParseCurrency` plus euro-specific handling.
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

Locale infrastructure exists.

## Ease of implementation

Moderate. Can largely reuse `LSParseCurrency` logic.

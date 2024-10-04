# Research: LSCurrencyFormat cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_lscurrencyformat()` in `src/cf8.cpp:14705` throws `"Function LSCurrencyFormat is not implemented"`.
- Compiler: `LSCURRENCYFORMAT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1742`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5535`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14705` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1742`, symbol at `src/compiler.cpp:5535` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*lscurrencyformat*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:614` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_LSCURRENCYFORMAT.md` |

## What LSCurrencyFormat does at the low C level

Per Adobe CF: format a number in a locale-specific currency format. Type (`local`, `international`, `none`) controls the currency symbol and grouping. Depends on locale infrastructure (already present via `cf_lsdateformat`/`cf_lstimeformat`).

- Arg 1: `number` (numeric).
- Arg 2: `type` (default `local`).
- Arg 3: `locale` (optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `number` (numeric). passed by value.
- Arg 2: `type` (default `local`). passed by value.
- Arg 3: `locale` (optional). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_lscurrencyformat(const cfvariant *number, const cfvariant *type, const cfvariant *locale);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- Full typed implementation using the existing locale infrastructure.
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

Locale infrastructure exists (`cf_lsdateformat`, `cf_lstimeformat` are implemented).

## Ease of implementation

Moderate. Locale-specific currency formatting rules (symbols, separators) need care; verify each locale behavior against ColdFusion.

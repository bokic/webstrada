# Research: LSParseNumber cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_lsparsenumber()` in `src/cf8.cpp:14784` throws `"Function LSParseNumber is not implemented"`.
- Compiler: `LSPARSENUMBER` is in the zero-arg not-implemented function list (`src/compiler.cpp:1742`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5546`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14784` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1742`, symbol at `src/compiler.cpp:5546` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*lsparsenumber*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:625` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (String/Format) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_LSPARSENUMBER.md` |

## What LSParseNumber does at the low C level

Per Adobe CF: convert a string that is a valid numeric representation in the current (or given) locale into a number. Locale-specific decimal/grouping separators are handled.

- Arg 1: `string` (input).
- Arg 2: `locale` (optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `string` (input). passed by value.
- Arg 2: `locale` (optional). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_lsparsenumber(const cfvariant *str, const cfvariant *locale);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- Full typed implementation using the existing locale infrastructure.
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

Locale infrastructure exists.

## Ease of implementation

Moderate. Needs locale-aware number parsing (decimal/group separators).

# Research: REMatchNoCase cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_rematchnocase()` in `src/cf8.cpp:15670` throws `"Function REMatchNoCase is not implemented"`.
- Compiler: `REMATCHNOCASE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5597`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15670` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5597` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*rematchnocase*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:687` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Regex) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_REMATCHNOCASE.md` |

## What REMatchNoCase does at the low C level

Per Adobe CF: case-insensitive variant of `REMatch`; returns an array of all matched substrings.

- Arg 1: `regex` (pattern).
- Arg 2: `string` (target).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `regex` (pattern). passed by value.
- Arg 2: `string` (target). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_rematchnocase(const cfvariant *regex, const cfvariant *str);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- **Regex engine** — none currently exists in the codebase. Case-insensitive mode needed.
- Array construction (array support already exists in cfvariant).
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

## Ease of implementation

Moderate-to-hard. Same effort as `REMatch`; only the case-insensitive flag differs.

# Research: SETPROFILESTRING cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_setprofilestring()` in `src/cf8.cpp:15817` throws `"Function SETPROFILESTRING is not implemented"`.
- Compiler: `SETPROFILESTRING` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5621`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15817` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5621` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*setprofilestring*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:719` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Get*/Meta/System) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SETPROFILESTRING.md` |

## What SETPROFILESTRING does at the low C level

Sets the value of a key in a (Windows) INI profile string or registry; returns the updated profile.

- Arg 1: `profileName` (string).
- Arg 2: `sectionName` (string).
- Arg 3: `entryName` (string).
- Arg 4: `value` (any).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `profileName` (string). passed by value.
- Arg 2: `sectionName` (string). passed by value.
- Arg 3: `entryName` (string). passed by value.
- Arg 4: `value` (any). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_setprofilestring(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4);
```
Compiled as a direct JIT call (AGENTS.md rule), registered via `AddSymbol`; the compiler emits the call site with `4` argument(s) evaluated into `const cfvariant *` parameters.

## Dependency

Function registry metadata; INI/profile file or registry access on Windows.

## Ease of implementation

Low-to-medium: single-purpose call, no state to maintain, but needs the above library. Real (typed) implementation can be done in one function.

# Research: REFind cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_refind()` in `src/cf8.cpp:15654` throws `"Function REFind is not implemented"`.
- Compiler: `REFIND` is in the zero-arg not-implemented function list (`src/compiler.cpp:1743`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5593`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15654` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1743`, symbol at `src/compiler.cpp:5593` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*refind*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:683` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Regex) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_REFIND.md` |

## What REFind does at the low C level

Per Adobe CF: case-sensitive regular-expression search of `string` for `regex`, starting at 1-based `start`. With `returnsubexpressions=false` returns the position of the first match (0 if no match). With `returnsubexpressions=true` returns a struct of arrays (`pos`, `len`, `match`) covering the match and its subexpressions.

- Arg 1: `regex` (pattern).
- Arg 2: `string` (target).
- Arg 3: `start` (optional, default 1).
- Arg 4: `returnsubexpressions` (optional boolean, default false).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `regex` (pattern). passed by value.
- Arg 2: `string` (target). passed by value.
- Arg 3: `start` (optional, default 1). passed by value.
- Arg 4: `returnsubexpressions` (optional boolean, default false). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_refind(const cfvariant *regex, const cfvariant *str,
                     const cfvariant *start, const cfvariant *returnSubexpressions);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- **Regex engine** — none currently exists in the codebase (no `std::regex`/PCRE usage found). Needs to be introduced (e.g. `std::regex` ECMAScript grammar, verified against Java-style CF regex).
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

## Ease of implementation

Moderate-to-hard. Core regex matching is straightforward with `std::regex`, but the struct-of-arrays subexpression return format and CF/Java regex dialect fidelity need care.

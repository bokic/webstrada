# Research: REReplace cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- Runtime stub: `cfml::cf_rereplace()` in `src/cf8.cpp:15697` throws `"Function REReplace is not implemented"`.
- Compiler: `REREPLACE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5601`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15697` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5601` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*rereplace*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:694` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Regex) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_REREPLACE.md` |

## What REReplace does at the low C level

Per Adobe CF: case-sensitive regex search-and-replace. `scope` can be `one` (default) or `all`; backreferences in the replacement use `\1` syntax.

- Arg 1: `regex` (pattern).
- Arg 2: `string` (target).
- Arg 3: `substring` (replacement, supports `\1` backreferences).
- Arg 4: `scope` (optional, `one`/`all`, default `one`).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `regex` (pattern). passed by value.
- Arg 2: `string` (target). passed by value.
- Arg 3: `substring` (replacement, supports `\1` backreferences). passed by value.
- Arg 4: `scope` (optional, `one`/`all`, default `one`). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_rereplace(const cfvariant *regex, const cfvariant *str,
                        const cfvariant *substring, const cfvariant *scope);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- **Regex engine** — none currently exists in the codebase. Needs replace-with-backreference support.
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

## Ease of implementation

Moderate-to-hard. Needs regex replace semantics plus `\1` backreference expansion; `scope` handling.

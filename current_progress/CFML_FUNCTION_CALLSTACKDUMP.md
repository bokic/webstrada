# Research: CALLSTACKDUMP cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_callstackdump()` in `src/cf8.cpp:11963` throws `"Function CALLSTACKDUMP is not implemented"`.
- Compiler: `CALLSTACKDUMP` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5266`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11963` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5266` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*callstackdump*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:282` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Get*/Meta/System) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CALLSTACKDUMP.md` |

## What CALLSTACKDUMP does at the low C level

Print the current call stack to the output (`browser` default, or `console`). Operates like WriteDump on the stack frames from `CallStackGet`.

- Arg 1: `output` (string, optional, default `browser`).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `output` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_callstackdump(const cfvariant *a1);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Needs a runtime call-stack tracker (function name + template + line per active call frame). No stack metadata is currently recorded. Unit tests + tracker updates.

## Ease of implementation

Moderate. Call-stack tracking must be added to the JIT invocation path; then it is a formatter emitting to the out buffer (WriteDump pattern).

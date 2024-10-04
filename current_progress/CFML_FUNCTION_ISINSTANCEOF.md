# Research: ISINSTANCEOF cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`

## Current state

- Runtime stub: `cfml::cf_isinstanceof()` in `src/cf8.cpp:14144` throws `"Function ISINSTANCEOF is not implemented"`.
- Compiler: `ISINSTANCEOF` is in the zero-arg not-implemented function list (`src/compiler.cpp:1740`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5483`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:14144` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1740`, symbol at `src/compiler.cpp:5483` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*isinstanceof*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:541` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Crypto/Token/Decision) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_ISINSTANCEOF.md` |

## What ISINSTANCEOF does at the low C level

Return true if `object` is an instance of (or implements) the Java type `typeName` (e.g. `java.util.Map`).

- Arg 1: `object` (any, required).
- Arg 2: `typeName` (string, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `object` (any). passed by value.
- Arg 2: `typeName` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_isinstanceof(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Java interop type-checking — no Java object model exists. Unit tests + tracker updates.

## Ease of implementation

Hard (or stub-throw in a native runtime).

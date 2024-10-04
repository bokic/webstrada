# Research: GETGATEWAYHELPER cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_getgatewayhelper()` in `src/cf8.cpp:13523` throws `"Function GETGATEWAYHELPER is not implemented"`.
- Compiler: `GETGATEWAYHELPER` is in the zero-arg not-implemented function list (`src/compiler.cpp:1736`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5369`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13523` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1736`, symbol at `src/compiler.cpp:5369` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*getgatewayhelper*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:418` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Get*/Meta/System) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETGATEWAYHELPER.md` |

## What GETGATEWAYHELPER does at the low C level

Return an instance of the helper object registered for the event gateway `gatewayID` (as configured in CF Administrator). Throws if the gateway is not defined.

- Arg 1: `gatewayID` (string, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `gatewayID` (string). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_getgatewayhelper(const cfvariant *a1);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Event-gateway configuration/registry does not exist. Unit tests + tracker updates.

## Ease of implementation

Hard. Requires the event-gateway subsystem and its configuration model.

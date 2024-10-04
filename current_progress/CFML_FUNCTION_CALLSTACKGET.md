# Research: CALLSTACKGET cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_callstackget()` in `src/cf8.cpp:11967` throws `"Function CALLSTACKGET is not implemented"`.
- Compiler: `CALLSTACKGET` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5267`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11967` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5267` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*callstackget*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:283` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Get*/Meta/System) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CALLSTACKGET.md` |

## What CALLSTACKGET does at the low C level

Return call-stack metadata: `type=array` gives an array of frame structs (`funcName`, `template`, `line`, `column`, `type`); `type=standard` returns a pipe-delimited string of `function@template:line` entries. `offset`/`maxFrames` slice the frames (defaults: from frame 0, all).

- Arg 1: `type` (string, optional, default `array`).
- Arg 2: `offset` (numeric, optional, default 0).
- Arg 3: `maxFrames` (numeric, optional, default 0).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `type` (string). passed by value.
- Arg 2: `offset` (numeric). passed by value.
- Arg 3: `maxFrames` (numeric). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_callstackget(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Runtime call-stack tracker (per-frame template/line/function) as for `CallStackDump`. Unit tests + tracker updates.

## Ease of implementation

Moderate. Same stack tracker as `CallStackDump`; array/struct assembly then mirrors the struct-of-structs formatting already used for queries.

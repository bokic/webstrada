# Research: QUERYEXECUTE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_queryexecute()` in `src/cf8.cpp:15175` throws `"Function QUERYEXECUTE is not implemented"`.
- Compiler: `QUERYEXECUTE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1743`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5581`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15175` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1743`, symbol at `src/compiler.cpp:5581` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*queryexecute*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:669` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Query*) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_QUERYEXECUTE.md` |

## What QUERYEXECUTE does at the low C level

Execute a SQL statement against a datasource and return the resulting query object. `params` supplies query-parameter bindings (named or positional, or an array of structs), `options` carries `datasource`, `timeout`, `result`, `dbtype`, etc. This is the script version of `<cfquery>`.

- Arg 1: `sql` (string, required).
- Arg 2: `params` (any, optional).
- Arg 3: `options` (struct, optional).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `sql` (string). passed by value.
- Arg 2: `params` (any). passed by value.
- Arg 3: `options` (struct). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_queryexecute(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Requires a SQL engine and datasource connectivity — the query engine for cfquery does not exist yet (cfquery is also unimplemented). Unit tests + tracker updates.

## Ease of implementation

Hard. Blocked on the entire database/SQL subsystem.

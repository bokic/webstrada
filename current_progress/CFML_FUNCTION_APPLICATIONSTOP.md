# Research: APPLICATIONSTOP cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_applicationstop()` in `src/cf8.cpp:11031` throws `"Function APPLICATIONSTOP is not implemented"`.
- Compiler: `APPLICATIONSTOP` is in the zero-arg not-implemented function list (`src/compiler.cpp:1732`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5239`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11031` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1732`, symbol at `src/compiler.cpp:5239` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*applicationstop*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:219` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Auth/SAML/OAuth) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_APPLICATIONSTOP.md` |

## What APPLICATIONSTOP does at the low C level

Stop the current application: the request throws an `ApplicationStopException` that unwinds to the request handler, clears the persistent `application` scope for this application name, and (by default) also clears the `session` scope of the current session. Subsequent requests to the same application start fresh. It is the function-form equivalent of `<cfapplication action="stop">`.

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_applicationstop();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

None — can be implemented with a request-level flag + clearing the `application`/`session` maps that the runtime already passes into compiled functions. Unit tests + tracker updates.

## Ease of implementation

Easy. Throw a dedicated exception (like `abort_exception`, `src/cf8.cpp:1812`) and have the request loop intercept it to clear scopes.

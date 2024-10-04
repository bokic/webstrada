# Research: SENDGATEWAYMESSAGE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_sendgatewaymessage()` in `src/cf8.cpp:15717` throws `"Function SENDGATEWAYMESSAGE is not implemented"`.
- Compiler: `SENDGATEWAYMESSAGE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1744`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5607`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:15717` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1744`, symbol at `src/compiler.cpp:5607` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*sendgatewaymessage*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:705` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_SENDGATEWAYMESSAGE.md` |

## What SENDGATEWAYMESSAGE does at the low C level

Send a message (the `data` struct, e.g. `to`/`text` for an SMS gateway) through the event gateway `gatewayID`. Returns a unique message ID.

- Arg 1: `gatewayID` (string, required).
- Arg 2: `data` (struct, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `gatewayID` (string). passed by value.
- Arg 2: `data` (struct). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_sendgatewaymessage(const cfvariant *a1, const cfvariant *a2);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Event-gateway subsystem + gateway configuration. Unit tests + tracker updates.

## Ease of implementation

Hard. Requires the event-gateway subsystem.

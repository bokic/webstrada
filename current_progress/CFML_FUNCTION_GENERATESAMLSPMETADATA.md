# Research: GENERATESAMLSPMETADATA cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_generatesamlspmetadata()` in `src/cf8.cpp:13418` throws `"Function GENERATESAMLSPMETADATA is not implemented"`.
- Compiler: `GENERATESAMLSPMETADATA` is in the zero-arg not-implemented function list (`src/compiler.cpp:1735`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5353`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:13418` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1735`, symbol at `src/compiler.cpp:5353` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*generatesamlspmetadata*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:397` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Auth/SAML/OAuth) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GENERATESAMLSPMETADATA.md` |

## What GENERATESAMLSPMETADATA does at the low C level

Generate the SAML 2.0 service-provider metadata XML document for the current application (entity descriptor with AssertionConsumerService endpoints, signing certificates, name-ID formats). Returns the metadata XML string.

- Arg: none.

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

This function takes no arguments.

## Proposed compiled form

```cpp
cfvariant *cf_generatesamlspmetadata();
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

SAML XML builder + certificate handling + app config. No SAML/crypto infra. Unit tests + tracker updates.

## Ease of implementation

Hard. Requires the full SAML SP metadata schema and certificate access; blocked on XML-writing and crypto support.

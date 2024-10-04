# Research: CANONICALIZE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_canonicalize()` in `src/cf8.cpp:11978` throws `"Function CANONICALIZE is not implemented"`.
- Compiler: `CANONICALIZE` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5269`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11978` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5269` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*canonicalize*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:285` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Security/Encode) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CANONICALIZE.md` |

## What CANONICALIZE does at the low C level

Repeatedly decode an encoded string until no more encodings are found (or until the configured max iteration count), applying OWASP ESAPI canonicalization. `restrictMultiple`/`restrictMixed` control whether multiple-encoded or mixed-encoding input is rejected; `throwOnError` makes it throw instead of returning the input.

- Arg 1: `input` (string, required).
- Arg 2: `restrictMultiple` (boolean, required).
- Arg 3: `restrictMixed` (boolean, required).
- Arg 4: `throwOnError` (boolean, optional, default false).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `input` (string). passed by value.
- Arg 2: `restrictMultiple` (boolean). passed by value.
- Arg 3: `restrictMixed` (boolean). passed by value.
- Arg 4: `throwOnError` (boolean). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_canonicalize(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3, const cfvariant *a4);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Needs a canonicalization engine (the `DecodeFor*` family) with per-encoding decoders and a decode-loop with a max-iteration guard. Unit tests + tracker updates.

## Ease of implementation

Moderate-to-hard. The decode loop is easy; a faithful ESAPI-compatible canonicalization engine with encoding detection needs an encoder library.

# Research: CREATEENCRYPTEDJWT cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_createencryptedjwt()` in `src/cf8.cpp:11989` throws `"Function CREATEENCRYPTEDJWT is not implemented"`.
- Compiler: `CREATEENCRYPTEDJWT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5274`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:11989` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5274` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*createencryptedjwt*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:297` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Crypto/Token/Decision) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CREATEENCRYPTEDJWT.md` |

## What CREATEENCRYPTEDJWT does at the low C level

Create an encrypted JSON Web Token (JWE) from a payload struct using the algorithm in `encryptOptions` (e.g. `RSA-OAEP`, `A128KW`) and signing/encryption keys from `config`. Returns the serialized JWE compact string.

- Arg 1: `payload` (any, required).
- Arg 2: `encryptOptions` (struct, required).
- Arg 3: `config` (struct, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `payload` (any). passed by value.
- Arg 2: `encryptOptions` (struct). passed by value.
- Arg 3: `config` (struct). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_createencryptedjwt(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

Needs an encryption/HMAC/JWT library (AES, RSA, SHA). No crypto support currently exists in the runtime. Unit tests + tracker updates.

## Ease of implementation

Hard. JWE is a full encryption framework (key management, content encryption, base64url, compact serialization); blocked on a crypto library.

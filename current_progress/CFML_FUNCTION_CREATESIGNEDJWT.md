# Research: CREATESIGNEDJWT cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`

## Current state

- Runtime stub: `cfml::cf_createsignedjwt()` in `src/cf8.cpp:12009` throws `"Function CREATESIGNEDJWT is not implemented"`.
- Compiler: `CREATESIGNEDJWT` is in the zero-arg not-implemented function list (`src/compiler.cpp:1733`).
- No interpreter (`evalFunction`) dispatch entry.
- Symbol registered at `src/compiler.cpp:5279`.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ❌ Stub that throws | `src/cf8.cpp:12009` |
| Compiler wiring | ⚠️ Compiled as zero-arg call into the not-implemented list; no args compiled/passed | `src/compiler.cpp:1733`, symbol at `src/compiler.cpp:5279` |
| Interpreter dispatch | ❌ Missing | — |
| Tag support | N/A (function only) | — |
| Tests | ❌ No `tests/cfm/*createsignedjwt*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ `PROGRESS.md:302` (❌ No), listed in `UNIMPLEMENTED_FUNCTIONS.md` (Crypto/Token/Decision) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CREATESIGNEDJWT.md` |

## What CREATESIGNEDJWT does at the low C level

Create a signed JSON Web Token (JWS) from a payload struct using the algorithm in `signOptions` (default `HS256`); `config` supplies the key/secret and claims (issuer, audience, expiry, etc.). Returns the JWT compact string.

- Arg 1: `payload` (any, required).
- Arg 2: `signOptions` (struct, required).
- Arg 3: `config` (struct, required).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller's variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller's variables are always unchanged after a call.

- Arg 1: `payload` (any). passed by value.
- Arg 2: `signOptions` (struct). passed by value.
- Arg 3: `config` (struct). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_createsignedjwt(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

HMAC-SHA/SHA family + base64url encoding; same missing crypto infrastructure as `CreateEncryptedJWT`. Unit tests + tracker updates.

## Ease of implementation

Moderate-to-hard. JWS is simpler than JWE (single signing key), but still requires HMAC/SHA support that does not exist yet.

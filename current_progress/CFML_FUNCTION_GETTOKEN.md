# Research: GetToken cffunction implementation notes

- Git commit: `065620009029231cadd3e54d0cc3c5b0f15affe7`
- Timestamp: `2026-08-03 20:06:00 UTC`
- Implemented: `2026-08-08` (Tier 1; see `tests/cfm/tier1_*.cfm` + `JitExpressionTest.Tier1*` unit tests)

## Current state

> Implemented (2026-08-08): `cf_gettoken` in `src/cffunctions/`, a direct JIT call in
> `src/codegen/codegen_expr.cpp` plus `cfvariant_call_function` / `evaluateExpr` dispatch,
> byte-verified against CF 2025 in `tests/cfm/tier1_*.cfm` and `JitExpressionTest.Tier1*`.

- Runtime stub: `cfml::cf_gettoken()` in `src/cf8.cpp:13669` throws `"Function GetToken is not implemented"`.
- Compiler: `GETTOKEN` is in the zero-arg not-implemented function list (`src/compiler.cpp:1737`).
- No interpreter (`evalFunction`) dispatch — calling it errors with `Unknown function call: gettoken`.
- Symbol registered at `src/compiler.cpp:5397`.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cffunctions/` |
| Compiler wiring | ✅ Native JIT handler | `src/codegen/codegen_expr.cpp` |
| Interpreter dispatch | ✅ `evaluateExpr` + `cfvariant_call_function` | `src/core/` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/tier1_*.cfm` + `JitExpressionTest.Tier1*` unit tests | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETTOKEN.md` |

Key detail: unlike the other 9 in this batch, GetToken has no implementation at all — fails both at compile time (zero-arg path) and in the interpreter.

## What GetToken does at the low C level

Per Adobe CF: return the token at 1-based `index` from `String`, splitting on the delimiter characters in `delimiters`. If `index` exceeds the number of tokens, return an empty string. Verified on real CF 2025: `getToken("a,b,c", 2)` returns `""` — the default delimiters are whitespace (space, tab, newline), so the local spec doc (`cfml_docs/CFML_FUNCTION_GETTOKEN.md`) stating `,` is wrong.

- Arg 1: `String` (input).
- Arg 2: `index` (1-based token position).
- Arg 3: `delimiters` (optional, default per spec `,`).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): ColdFusion passes simple-value arguments (string, number, boolean) **by value** — mutating a parameter inside a function never changes the caller’s variable. Structs and queries are passed by reference and arrays by value, but this function takes only simple arguments, so the caller’s variables are always unchanged after a call.

- Arg 1: `String` (input). passed by value.
- Arg 2: `index` (1-based token position). passed by value.
- Arg 3: `delimiters` (optional, default per spec `,`). passed by value.

## Proposed compiled form

```cpp
cfvariant *cf_gettoken(const cfvariant *str, const cfvariant *index, const cfvariant *delimiters);
```

Requires a typed implementation, a native JIT handler (like `cf_writedump`, `src/compiler.cpp:1762`), and removal from the zero-arg not-implemented list.

## Dependency

- Full typed implementation of the function (currently just a stub).
- Native JIT compile handler in `src/compiler.cpp` + removal from the zero-arg not-implemented list.
- Interpreter dispatch entry in `evalFunction`.
- Unit tests (`tests/cfm/`) + `verify_with_coldfusion.py` verification, especially the default-delimiters behavior.
- Tracker updates (`PROGRESS.md`, `UNIMPLEMENTED_FUNCTIONS.md`).

No runtime infrastructure dependencies.

## Ease of implementation

Easy. Self-contained string parsing; the only real work is writing the tokenizer and confirming default delimiters against ColdFusion.

# Research: ISDEFINED cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120` (original stub), implemented on top.
- Timestamp: `2026-08-05` (implemented)

## Current state

- Runtime: `cfml::cf_isdefined(const cfvariant *a1, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables)` in `src/cf8.cpp` — resolves a dotted variable name against the runtime scope maps.
- Compiler: `ISDEFINED` is removed from the zero-arg not-implemented list and gets a direct 9-arg JIT compile case (`src/compiler.cpp`) that passes the 8 scope pointers + the evaluated name.
- Interpreter: dispatched in both `evaluateExpr` (cfoutput `#...#` path) and `cfvariant_call_function` (runtime dispatch).
- Symbol registered at `src/compiler.cpp` (`cf_isdefined`, 9 ptr args).
- Tests: `tests/cfm/isdefined_test.cfm` verified byte-for-byte against CF 2025, plus `JitExpressionTest.IsDefined*` unit tests (9 tests).

## Implemented: 100%

## Behavior (verified on Adobe ColdFusion 2025)

`IsDefined(variable_name: string) -> boolean`

- Takes exactly 1 argument; a wrong arg count throws `IsDefined requires exactly 1 argument`.
- Argument stringifies; empty string → NO.
- Unqualified names: searched only in the `variables` scope (plus, inside a UDF, the captured parent scopes via `g_udfCtx`) — NOT session/application/request/form/url/cookie (matching CF).
- Scope-prefixed names: `variables`, `url`, `form`, `cookie`, `server`, `application`, `session`, `cgi` resolve their member chain case-insensitively; scope names themselves (`session`, `form`, ...) are YES.
- Inside a UDF: `arguments.*` (the function's ARGUMENTS struct) and `local.*` (mapped to the current UDF local scope via `g_udfCtx.back().localScope`) resolve; bare `arguments`/`local` are YES inside a function and NO at page level.
- Null handling: a variable or struct member holding null/NotSet is NOT defined (CF: `IsDefined("nullvar")` → NO).
- Query columns resolve like struct members (`IsDefined("q.name")` → YES, `q.nope` → NO).
- CGI quirk: **any** `cgi.<member>` reference returns YES even for non-existent keys (`cgi.NOPE`, `cgi.`, `cgi.a.b`) — reproduced.
- `REQUEST` scope is not implemented in the engine, so `IsDefined("request.*")` returns NO (CF: YES); documented in BUGS.md.

## CF-side probe notes

- `IsDefined(nullvar)` where the *argument value* is null truncates the CF response mid-render (server-side chunked-encoding issue; the verify suite's IncompleteRead path) — not a WebStrada difference.
- `IsDefined("arr[1]")` (square-bracket syntax) also truncates the CF response; per CF docs square-bracket notation is not supported. Both are excluded from the CF verification test.
- `cookie.CFID` is YES on CF (session cookie minted in-request) but NO under `WebStrada-cli` (no cookie minting in the CLI); cookie cases are excluded from the byte-for-byte test.

## Compiled form

```cpp
cfvariant *cf_isdefined(const cfvariant *a1, void *cgi, void *server, void *cookie,
                        void *application, void *session, void *url, void *form,
                        void *variables);
```

Direct JIT call (no dynamic lookup), passing the 8 scope pointers like `cfvariant_get_var`.

# Research: GETHTTPREQUESTDATA cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`
- Implemented: `2026-08-06` (see `tests/cfm/gethttprequestdata_test.cfm`, byte-matched against CF 2025)

## Current state

- Runtime: `cfml::cf_gethttprequestdata(void *cgi, const cfvariant *includeBody)` in `src/cf8.cpp` — builds a struct from the CGI scope (`headers` from the HTTP_* vars, `protocol` from SERVER_PROTOCOL, `method` from REQUEST_METHOD) plus the request body captured by the daemon worker (`request_set_body`), honoring the `includeBody` argument (default true; false omits the `content` key entirely).
- Compiler: `GETHTTPREQUESTDATA` has a dedicated native JIT handler in `src/compiler.cpp` that passes the live CGI scope pointer plus the evaluated `includeBody` argument (no dynamic lookup). It was removed from the zero-arg not-implemented list.
- Interpreter: `evaluateExpr`'s function-call dispatch handles `GETHTTPREQUESTDATA` (passing the `cgi` pointer), and `cfvariant_call_function` too.
- Symbol registered at `src/compiler.cpp` (AddSymbol `cf_gethttprequestdata`).

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cf8.cpp` `cfml::cf_gethttprequestdata` |
| Compiler wiring | ✅ Native JIT handler (cgi scope + includeBody) | `src/compiler.cpp` |
| Interpreter dispatch | ✅ `GETHTTPREQUESTDATA` case in `evaluateExpr` + `cfvariant_call_function` | `src/cf8.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/gethttprequestdata_test.cfm` + `JitExpressionTest.GetHttpRequestData` unit tests, verified against CF 2025 | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETHTTPREQUESTDATA.md` |

## What GETHTTPREQUESTDATA does at the low C level

Return a struct of the current HTTP request's data with keys `headers`, `protocol`, `method`, `content` (in that insertion order, matching CF). `includeBody=false` omits the `content` key.

- Arg 1: `includeBody` (boolean, default true — verified on CF 2025: `GetHttpRequestData()` includes content, `GetHttpRequestData(false)` omits the key).

## Parameter passing

Simple-value arguments are passed by value; the function only takes an optional boolean.

## Proposed compiled form

```cpp
cfvariant *cf_gethttprequestdata(void *cgi, const cfvariant *includeBody);
```

## Dependency

The runtime needs the CGI scope (passed by the JIT) and the raw request body, which the daemon worker captures per request (`request_set_body`); the CLI has no body.

## Ease of implementation

Moderate. Assemble from the existing CGI scope; the request body capture was added to the worker.

# Research: Trace cffunction implementation notes

- Git commit: `3b7f00a9dd5b9ca6f772d727088c019cdb17800e`
- Timestamp: `2026-08-03 20:06:00 UTC`

## Current state

- `cfml::cf_trace()` in `src/cf8.cpp:16770` is a stub that throws `"Function Trace is not implemented"`.
- The `cftrace` tag is in the not-implemented tag list at `src/compiler.cpp:4236`.
- Related stubs: `cf_isdebugmode()` (`src/cf8.cpp:14125`) and `cflog` are also unimplemented.

## Implemented: 0%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime stub | ❌ Throws `"Function Trace is not implemented"` | `src/cf8.cpp:16770` |
| Compiler wiring | ⚠️ `trace()` compiles, but as a zero-arg call to `cf_trace` (no arguments compiled/passed) | `src/compiler.cpp:1748` (not-implemented function list), symbol registered at `src/compiler.cpp:5726` |
| Tag support | ❌ `cftrace` tag in not-implemented tag list | `src/compiler.cpp:4236` |
| Tests | ❌ No `tests/cfm/*trace*`, no `verify_with_coldfusion.py` coverage | — |
| Tracker status | ❌ Listed as unimplemented | `PROGRESS.md:841` (Trace ❌ No), `UNIMPLEMENTED_FUNCTIONS.md` (Create/Misc group) |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_TRACE.md`, `cfml_docs/CFML_TAG_CFTRACE.md` |

Key detail: `trace()` compiles successfully — the failure happens at runtime, not at compile time.

## What Trace does at the low C level

In Adobe ColdFusion, a faithful implementation must do the following per invocation:

1. **Compute execution time** since the request started (trace output always shows elapsed ms).
2. **Evaluate args**: `var` (a live variable reference, can be complex), `text`, `type` (information/warning/error/fatal), `category`.
3. **Append to a per-request trace buffer** rendered in the debugging section at the end of the request (the primary "trace" output).
4. **If `inline=true`**: emit an HTML trace table directly into the response `out` buffer at the tag's location — the same mechanism `cf_emit_writedump(void *out, ...)` uses for WriteDump (`src/cf8.cpp:1800`).
5. **If `abort=true`**: throw `abort_exception`, same path WriteDump uses (`src/cf8.cpp:1812`).
6. **Log to `logs/cftrace.log`** — but only when debugging is enabled (Administrator setting).

## Parameter passing (by value / by reference)

Verified on Adobe ColdFusion 2025 (`RDS_HOST=192.168.100.10`): simple-type arguments (`text`, `type`, `category`, `inline`, `abort`) are passed **by value**. The `var` argument may be complex: structs and queries are passed by reference and arrays by value, but Trace only reads/prints `var` (it never mutates it), so the caller’s variable is always unchanged after the call.

- Arg `var`: a live variable reference; may be complex (struct/query/array).
- Arg `text`: a string.
- Arg `type`: one of `information`, `warning`, `error`, `fatal`.
- Arg `category`: a string.
- Arg `inline`: a boolean.
- Arg `abort`: a boolean.

### Proposed compiled form

Per AGENTS.md, it should be compiled as a direct JIT call (like `cf_writedump`, `src/compiler.cpp:1762`) with a signature roughly:

```cpp
cfvariant *cf_trace(string &out, cfvariant *var, cfvariant *text, cfvariant *type,
                    cfvariant *category, cfvariant *inline, cfvariant *abort);
```

The core — formatting a string, appending to `out`, conditionally throwing abort — is basically a trimmed-down WriteDump.

## Dependency

Things this feature needs that are not currently present in the codebase:

| Dependency | Needed for | Current state |
|-----------|-----------|---------------|
| Per-request trace buffer | Holding trace entries for the end-of-request debug summary | ❌ Does not exist |
| Request start timestamp | Computing elapsed execution time shown in trace output | ❌ Does not exist (no request-context plumbing) |
| Debug-mode flag | CF gates all trace output on debugging being enabled | ❌ `cf_isdebugmode()` is a stub (`src/cf8.cpp:14125`) |
| File-logging infra | Writing `logs/cftrace.log` | ❌ `cflog` unimplemented (`src/compiler.cpp:4228`) |
| Request-context plumbing | Passing start time / trace buffer / debug flag through to the JIT runtime | ❌ Does not exist |

Note: without these, only the partial subset (`inline` + `abort` + `text`/`var` dump, reusing the WriteDump pattern) is implementable.

## Ease of implementation

**Core subset (easy):** `inline`/`abort`/`text`/`var` can be done quickly by reusing the WriteDump pattern (`cf_writedump` / `cf_emit_writedump`).

**Full CF-faithful behavior (needs infra first):** blocked on the missing infrastructure listed in the [Dependency](#dependency) section above.

Conclusion: quick win for a partial version; the full behavior needs a small request-context plumbing step (trace buffer + request start time + debug flag) first.

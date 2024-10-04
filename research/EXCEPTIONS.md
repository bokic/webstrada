# Research: Implementing try/catch (cfscript + cftags) in WebStrada

## Summary

Parsing-side support is mostly done already. The real work is the JIT
exception-handling mechanism. Crucially, the foundation already works: every
runtime error is a C++ `webstrada::exception` thrown from a runtime function
(`src/cf8.cpp`, `src/cfvariant.cpp`) called by JIT code, and these exceptions
already unwind through the JIT-compiled frames to the top-level handler. The
`.eh_frame` unwind info that MCJIT emits is registered with the unwinder, so
the unwinder can walk JIT frames. Implementing try/catch therefore means adding
**landing pads** on top of existing working unwinding, not building unwinding
from scratch.

## Current state

- `cfthrow`, `cfrethrow`, `cftry`, `cfcatch`, `cffinally` are all `✅ Yes` in
  PROGRESS.md for both the **script** (`throw`/`rethrow`/`try`/`catch`/`finally`)
  and **tag** (`<cfthrow>`/`<cfrethrow>`/`<cftry>`/`<cfcatch>`/`<cffinally>`)
  forms, verified byte-for-byte against CF 2025.
- The exception model is `webstrada::exception` with `m_type`/`m_message`/
  `m_detail`/`m_errorCode`/`m_extendedInfo` plus `abort_exception`/
  `template_exception` subclasses (`include/webstrada/exceptions.h`).
- Runtime EH helpers live in `src/cf8.cpp`: `cf_eh_capture` (snapshot a caught
  exception into a CFML struct, `__cxa_begin_catch`/`__cxa_end_catch`),
  `cf_eh_matches` (non-consuming type test, case-insensitive, "any" matches,
  uncatchable → false), and `cf_eh_throw` (raise a fresh `webstrada::exception`
  from a struct with TYPE/MESSAGE/DETAIL/ERRORCODE/EXTENDEDINFO, type defaults
  to `Application`).
- Script `throw` is compiled by `compile_script_throw_statement`: it builds the
  attribute struct in IR (`cfvariant_create_struct` + `cfvariant_index_assign`)
  for both named-attribute and bare/message forms, then emits
  `cf_eh_throw(struct)` + `CreateUnreachable()`. Script `rethrow;` emits
  `cf_eh_throw(captured)` where `captured` is `g_currentCatchExn`, a thread_local
  set/restored around catch-body compilation so it always names the innermost
  enclosing catch.
- The compiler emits every call inside a try body as an `invoke` targeting the
  landing pad; calls outside any try are plain `CreateCall`. The C++ Itanium
  personality (`__gxx_personality_v0`) is attached to every JIT function.
- The top-level handler catches `abort_exception`, `template_exception`,
  `webstrada::exception`, `std::runtime_error`, `std::exception`, and `...`
  (`src/worker.cpp:257`).

### Verified facts

- `<cfset x = 1 / 0>` -> `Error: Division by zero`
- `<cfset y = undefinedvar>` -> `Error: Variable UNDEFINEDVAR is undefined.`
- `<cfset a = [1,2]><cfset z = a[10]>` -> `Error: Array index out of bounds`
- `<cfabort>AFTER` stops the template (no `AFTER` output), proving
  `abort_exception` propagates through the JIT `main` frame to the C++ handler.

These confirm that C++ exceptions already unwind through JIT frames, i.e.
`.eh_frame` registration is working.

## Parsing side — mostly ready

### cfscript

- `try` and `catch` are already keywords in the grammar
  (`definitions/cfml_definition.json:378`, `startRegex` of `Keyword`).
- `finally` is NOT a keyword; it tokenizes as a `Variable` (fine — dispatch on
  token text like `do` does today).
- The script statement compiler (`src/compiler.cpp` around line 3948) dispatches
  on `if`/`function`/`return`/`for`/`while`/`switch`/`do`; there is no case for
  `try`/`catch`/`finally` yet. Add dispatch cases there.

### cftag

- `<cftry>`, `<cfcatch type=...>`, `<cffinally>` currently hit the "Unknown
  start tag" error path (`src/compiler.cpp:4754`). Generic tag-pair parsing
  already exists, so new cases in `compile_token_list` are all that's needed.
- `<cfcatch>`/`<cffinally>` may only appear inside `<cftry>`; the compiler must
  enforce that (analogous to `<cfcase>` -> "only valid inside a cfswitch
  block").

## The real work: JIT exception handling

Two viable strategies, with a big effort difference.

### Strategy 1 — Function-wrapping (easiest, ~1-2 days)

Compile each `try` body as a separate JIT function and emit a single
`invoke` + landing pad at the `try` statement.

- The machinery for compiling a body into a fresh JIT function already exists:
  `compileUdfFunction` (`src/compiler.cpp:3084`).
- The body function is compiled with all existing plain `call`s; when a nested
  runtime call throws, the unwinder walks out through the body function's
  `.eh_frame` and the ONE landing pad catches it.
- Very localized change: only the `try` statement emission site needs the
  `invoke`/landingpad.

Caveats:
- `return`/`break`/`continue` inside the try body now operate on the body
  function's frame, not the enclosing function — semantics must be threaded out
  (e.g. via a shared result slot).
- `var`/`local` scoping: the body function must share the enclosing scope.
  Since scopes are passed by pointer (the `variables`/local struct pointer), a
  body fn taking the same scope pointers should behave correctly for
  reads/writes; local-name marking must be consistent.

### Strategy 2 — Full native EH (cleaner long-term, ~1 week)

Convert `call` -> `invoke` for every potentially-throwing call inside a try
region.

- Requires threading a "current try region" context (landing pad / cleanup
  info) through all call emission: `CompileExprAST` and all statement
  compilers, hundreds of call sites.
- Needs personality function setup (`__gxx_personality_v0`) on generated
  functions, and `__cxa_begin_catch`/`__cxa_end_catch`/`llvm.eh.exceptionpointer`
  to extract the `webstrada::exception*`.
- Correct for all nesting depths and scoping, but a large, invasive, mechanical
  change.

Note: intermediate JIT frames without their own landing pads are fine — the
unwinder only needs the FIRST frame with a matching landing pad to catch, and
every frame on the path needs unwind info (which MCJIT already emits).

## Runtime / semantics work — moderate, mechanical

- The exception model needs a CF exception type string (`application`,
  `expression`, `custom`, `database`, ...). Currently only message + detail
  exist. Catch-type matching in CF is by this type string; `catch(type="any")`
  matches everything.
- Implement `cfthrow` (currently a stub) with `type`/`message`/`detail`/
  `errorCode`/`extendedInfo` attributes/args.
- Implement `cfrethrow` (tag) and `rethrow;` (script) with correct
  rethrow-through-finally semantics.
- Implement `cffinally` and script `finally` (its body runs on both paths).
- Build the `cfcatch` struct for the tag form (TYPE, MESSAGE, DETAIL,
  tagContext, ...) and assign the exception to the script `catch (type var)`
  variable.
- `cfabort`'s `abort_exception` is currently swallowed silently; decide whether
  `catch` can intercept it (CF: `catch` does NOT catch `<cfabort>`; only
  `onRequestEnd` handles it — must keep `abort_exception` propagating past
  catch handlers).

## Verification

- Per AGENTS.md, add `tests/cfm` files with as many variants as possible
  (type matching, any, nested try, finally, rethrow, catch in UDF, throw inside
  catch, etc.) and verify byte-for-byte against CF 2021 with
  `./tests/verify_with_coldfusion.py`.

## Implementation notes (Strategy 2 done for cfscript try/catch/finally)

Completed (see PROGRESS.md `cftry`/`cfcatch` rows):

- **Personality**: `__gxx_personality_v0` is resolved via
  `dlsym(RTLD_DEFAULT, ...)` and registered with
  `llvm::sys::DynamicLibrary::AddSymbol`; `getOrCreatePersonality()` attaches it
  to every JIT function (any single `try` already triggers this).
- **`emitCall` / `EhContext`**: all ~155 runtime-call emission sites now go
  through `emitCall(builder, f, args)`. A `thread_local EhContext*` holds the
  active landing-pad block; while compiling a try body it is set, so every call
  becomes an `invoke` (normal edge = continuation, unwind edge = landing pad).
- **Statement shape** (verified against the textparser token stream): `try` is
  a `Keyword` (id 37), bodies are `CodeBlock` (36), `catch (type var)` is a
  `Parenthesis` (38) whose single `ScriptExpression` (22) child holds the type +
  variable `Variable` (34) tokens (dotted type spans first..last identifier),
  and `finally` tokenizes as a `Variable`. Comments between keywords and blocks
  are skipped.
- **Landing pad**: `{ptr, i32}` struct, catch-all clause
  (`ConstantPointerNull`), element 0 = exception pointer. Dispatch is done by
  `cf_eh_matches(exn, typeStr)` which does **not** consume the exception, so a
  no-match keeps unwinding; the matching clause then binds its var via
  `cf_eh_capture` (does `__cxa_begin_catch`/`__cxa_end_catch`, builds the
  TYPE/MESSAGE/DETAIL/ERRORCODE/EXTENDEDINFO/TAGCONTEXT struct, pushes it as a
  temp variant). `catch (any)` matches every catchable exception; uncatchable
  (`abort_exception`) never matches.
- **`finally` + re-raise**: an uncaught exception is snapshotted
  (`cf_eh_capture`) on the unmatched path, the finally body runs in a dedicated
  rethrow copy, then `cf_eh_throw` raises a fresh exception carrying the same
  type/message/detail/errorcode/extendedinfo. `cf_eh_throw`'s default type is
  `"Application"` when the struct has no TYPE field.
- **Built-in exception types use ColdFusion's canonical casing**:
  `Expression`, `Application`, `Request`, `Template`; user `cfthrow` types keep
  their own casing (verified: `cfcatch.type` for `1/0` is `Expression`, for a
  `<cfthrow type="myCustom">` is `myCustom`).

### Why not `resume`?

The first implementation re-raised unmatched exceptions with an LLVM `resume`
instruction (re-enter unwinding with the original landing-pad value). It
miscompiled badly in the MCJIT: with a shared finally block whose success edges
fed a `{ptr,i32}` phi, the exception was silently **swallowed** (finally ran,
then the code fell through instead of rethrowing — `end` printed, exit=0).
Duplicating the finally body so the `resume` operand was dominated did not fix
it either (SIGSEGV/SIGILL). Only a direct `resume` immediately after the
landing pad (no finally) worked. The robust, always-correct approach is the
capture-then-rethrow sequence above, which never touches `resume`.

### Known divergence (CLI only)

`WebStrada-cli` writes the request output only after a successful compile+run,
so output buffered before an *uncaught* exception (including a `finally` block's
writes on the unmatched path) is not printed — matching how the CLI already
treats every uncaught error. Semantics are verified instead by re-catching the
re-raised exception in an outer `try` (see `tests/cfm/cfscript_try_catch_test.cfm`
case 8/9).

### Current state (script + tag forms complete)

- `cfthrow`, `cfrethrow`, `cftry`, `cfcatch`, `cffinally` are all `✅ Yes` in
  PROGRESS.md for both the **script** (`throw`/`rethrow`/`try`/`catch`/`finally`)
  and **tag** (`<cfthrow>`/`<cfrethrow>`/`<cftry>`/`<cfcatch>`/`<cffinally>`)
  forms, verified byte-for-byte against CF 2025.

### Remaining work

- All try/catch/throw forms are implemented. Remaining divergences are
  documented in BUGS.md: script `throw 5+3` evaluates the whole expression
  (CF takes one operand), struct/array literal args throw an empty Application
  exception instead of aborting the page, `rethrow;`/`<cfrethrow>` outside a
  catch is a compile error instead of a silent abort, and the CF 2025 test
  server itself returns a server-error for `<cfloop>` directly inside
  `<cftry>` (this engine handles it fine).
- The tag form shares the EH codegen with the script form via
  `emit_try_catch_codegen`; `<cfthrow>` attribute values are compiled from the
  tag's Expression children so quoted values interpolate `#expr#`.

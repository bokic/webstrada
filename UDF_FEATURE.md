# UDF Feature — User-Defined Functions and Closures

Status: **Implemented** (compile-time registration + runtime dispatch + closures),
verified byte-for-byte against ColdFusion 2021 in `tests/cfm/udf_basic_test.cfm`,
`udf_closure_test.cfm`, `udf_scope_test.cfm` and the `UdfTest` unit-test suite.
BUGS.md item #1 (closures degrading to a bare parameter reference) is fixed.

This document is the single source of truth for the "register and call custom
functions" feature (cfscript `function` keyword + anonymous function/closure
expressions). It captures the exact Adobe ColdFusion 2021 behavior, verified by
running snippets against the local CF server, and the resulting implementation
design for WebStrada.

All experiment snippets live in `tmp/udf_probes/e*.cfm`; run one against the
server with `./tmp/run_cf.sh tmp/udf_probes/eNN.cfm`. The CF error messages were
extracted from `/opt/coldfusion/cfusion/lib/cfusion.jar` resource files on the
server (copy kept in `tmp/cflogs/`).

---

## 1. Goal

1. **Fix BUGS.md #1** — `function(x) { ... }` closures in expressions must
   compile as a single closure *value* (consuming `function`, the `(...)`
   parameter list and the `{ ... }` body), not degrade to a bare parameter
   reference.
2. **Register custom functions** — `function name(params) { body }` statements
   in cfscript register a callable UDF under `name`.
3. **Call custom functions** — `name(args)` must dispatch to a registered UDF;
   a closure/function *value* stored in a variable, struct, or passed as an
   argument must be invocable (`f(1,2)`, `s.fn(1)`, `arrayMap(arr, fn)`).

UDFs behave like built-ins to the caller; the difference is only that the
compiler cannot know a UDF's address at template-compile time, so a runtime
function-name check resolves it.

---

## 2. Current WebStrada state (what must change)

| Area | Current behavior | Problem |
|------|------------------|---------|
| Closure in expression | `parseTokensToAST` (src/compiler.cpp:762) tokenizes `function(x)` as a `TextParser_cfml_Function` token and treats it as a normal call `function(x)`; the trailing `{ ... }` CodeBlock is never consumed. IR becomes `cfvariant_call_function(out, ..., "arrayMap", [arr, x], 2)` → "Variable X is undefined." | The `function` keyword + body are dropped; only `(x)` survives |
| Function declaration statement | `compile_script_statement` (src/compiler.cpp:3274) has no `function name(...){...}` case; it falls into the generic expression path and misparses | `function add(a,b){...}` → "Unknown function call: add" |
| Function value | `cfvariant::Function` (include/webstrada/cfvariant.h) is only a *built-in method handle* — a text string (cf8.cpp `functionHandleText`). Not callable, no compiled code, no metadata | UDF/closure values can't be stored or invoked |
| Call dispatch | Compile-time-known builtins → direct JIT stub; everything else → `cfml::cfvariant_call_function` (src/cf8.cpp:7084) string lookup → "Unknown function call: NAME" (cf8.cpp:5013) | No UDF/variable lookup path |
| Callbacks | `ArrayMap`/`ArraySort` etc. invoke named string callbacks via `callCallback`/`evaluateExpr` (cf8.cpp:1908); a `Function` value's `toString()` is not a callable name | `arrayMap(arr, function(x){...})` unusable |
| Bare name | `cfvariant_bare_identifier` (cf8.cpp:6751): undefined variable → built-in handle or "Variable X is undefined." | No UDF resolution |

---

## 3. Verified ColdFusion 2021 behavior (source of truth)

Tests: `tmp/udf_probes/e*.cfm`; server: CF 2021 at `192.168.100.10:8500`.
Server errors abort the HTTP response, so "page aborted" below means a fatal
runtime/compile error; exact exception classes/messages were recovered from
`coldfusion-error.log` and `cfusion.jar`.

### 3.1 Registration, storage, visibility

- **UDFs live in the `variables` scope.** `StructKeyExists(variables,"pageFunc")`
  → `YES`, `IsDefined("pageFunc")` → `YES`, `IsCustomFunction(variables.pageFunc)`
  → `YES`. (e23)
- **Hoisted across the whole template.** A UDF is callable from `<cfoutput>` and
  from code that textually precedes its definition: `<cfoutput>#earlyCall(5)#</cfoutput>`
  then `function earlyCall(n)` → `E5`; `isNull(noReturn())` called before
  `function noReturn(){}` → `YES`. (e20, e10)
- **Cannot shadow a built-in name.** `function len(s){}` (len is built-in) →
  compile-time `ASTfunctionDefinition.InvalidUDFNameException`:
  "The names of user-defined functions cannot be the same as built-in ColdFusion
  functions." (detail: "The name {udfname} is the name of a built-in ColdFusion
  function."). So a name is *either* a built-in *or* a UDF — never both. (e11d)
- **Duplicate definitions are a compile error.** Two `function dup(){}` →
  `NeoTranslationContext.StaticDuplicateFunctionDefinitionException`:
  "Routines cannot be declared more than once." (detail: "The routine {routineName}
  has been declared twice in the same file."). (e21c)
- **Functions defined inside an `if` block are hoisted** to the enclosing
  function scope (`condFn()` callable). (e24)
- **Nested functions are scoped to the enclosing function** — `inner()` defined
  inside `outer()` is not visible at page level ("Variable INNER is undefined."). (e24)
- UDF name restriction: not a built-in name (see above). Illegal-char names →
  `IllegalUDFNameException` "Invalid name for user-defined function."

### 3.2 Calling semantics

- **No arity checking.** Extra positional args are accepted and land only in
  `arguments`; missing args make the param *undefined* — using it throws
  `UndefinedVariableException: Variable {PARAM} is undefined.` (param name
  upper-cased). `add(2,3,4)` → 5 (extra ignored); `add(2)` → "Variable B is
  undefined."; `add()` → "Variable A is undefined.". (e1)
- **Default parameter values** (`b = 10`, `c = "x"`, `d = []`,
  `e = structNew()`) are evaluated at call time when the arg is absent. (e2, e21)
- **Named arguments are NOT valid in cfscript calls.** `person(age=30, name="Bob")`
  → `compiler.ParseException` "Invalid CFML construct found on line {line} at
  column {column}." (e16a/b)
- **`arguments` scope** is a struct containing upper-cased param-name keys **and**
  numeric indices 1..N of *all* passed args. `structKeyList(arguments)` → "A,B";
  `arguments[1]`, `arguments.a`, `arrayLen(arguments)`, `structCount(arguments)`
  all work. (e18, e22, e23)
- **Calling a non-existent function** is not a "function not found" error — it
  resolves the name as a *variable* and fails with
  `UndefinedVariableException: Variable {NAME} is undefined.` (e19, from log:
  `totallyUndefinedFn(1,2)` → "Variable TOTALLYUNDEFINEDFN is undefined.")
- **Recursion works** (`fact(5)` → 120). (e13)
- **Functions are first-class values**: `f2 = pageFn; f2(4)`; store in a struct
  `s.fn = function(){}; s.fn(1)`; pass as argument `invokeIt(pageFn, 5)`; call
  via scope `variables.person("X",1)`. (e16c, e25)

### 3.3 Type checking and return types

Declared param/return types are **coerced**, not strictly enforced:
- `function sq(numeric n)` called `sq("5")` → 25, `sq(true)` → 1, `sq(s)` with
  `s="5"` → 25. (e5, e19b)
- A *non-convertible* value (`sq("abc")`, literal or variable) **aborts the
  whole page** with `UDFMethod.InvalidArgumentTypeException`:
  "The {arg} argument passed to the {functionName} function is not of type
  {expectedType}." — **not catchable** by `try/catch`, no output after the call. (e5d/e5f)
- Return type coercion: `returntype="string"` returning `42` → `"42"`;
  `returntype="numeric"` returning `"7"` → `7`. (e3)
- Non-convertible return (`returntype="numeric"` returning `"abc"`) aborts with
  `UDFMethod.InvalidReturnTypeException`: "The value returned from the
  {functionName} function is not of type {type}." (e4)
- `returntype="void"` that `return`s a value aborts with
  `UDFMethod.IllegalReturnException`: "The function {functionName} defined as
  void tried to return a value." (e17)

### 3.4 Closures (anonymous functions)

- `f = function(x) { return x + 1; }; f(41)` → 42. (e11b)
- `IsClosure(anon)` → `YES`; `IsCustomFunction(anon)` → `YES`. (e11c)
- A **named UDF** `person` → `IsClosure(person)` → `NO`; `IsCustomFunction` →
  `YES`. So named UDFs are "custom functions" but not "closures". (e16c)
- Closures are the standard callback mechanism: `arrayMap(arr, function(x){return x*2;})`
  → 2,4,6,8; `arrayFilter(arr, function(x){return x GT 2;})` → 3,4. (e18)
- **Closures capture a scope instance**:
  - Capture page vars: `base=100; adder=function(x){return x+base;}; adder(1)` → 101. (e15)
  - Per-invocation state persists: a counter closure returns 1,2,3 across calls. (e15)
  - Per-invocation params are captured: `makeFactory(base2)` returns closures
    where `f5(3)` → 15 and `f7(3)` → 21 independently. (e15)
- A page-level closure's unqualified assignment leaks to page variables
  (`fn=function(x){ leaky=x*2; return leaky; }` → page `leaky` = 10). (e22)

### 3.5 Variable scoping inside UDFs/closures (the classic CF gotcha)

Verified with `var` vs unqualified access (e13, e14, e26):
- `var x` → **function-local** scope; shadows page `x`; never leaks.
  `withVar()` var'd `secret`/`j` are undefined at page level.
- Unqualified assignment inside a UDF **writes to the calling page's `variables`
  scope** (`localVar=5` leaked; `y=5` overwrote page `y`).
- Unqualified read: local `var` scope first, then the enclosing (page) scope
  (`usesOuter()` reads page `outer`; `seesX()` reads current page `x` = 42, i.e.
  **dynamically**, not a snapshot at definition). (e21e)
- A named UDF therefore resolves enclosing names **dynamically**, whereas a
  closure captures a specific scope **instance** (see 3.4).

### 3.6 Miscellaneous

- `return null;` is **not** valid CF 2021 — `null` is treated as an undefined
  variable name: `UndefinedVariableException: Variable NULL is undefined.`. (e9)
  (WebStrada currently supports a `null` keyword; this is a divergence to be
  aware of, but the runtime `cfvariant::Null` type stays.)
- A function with no `return` (or a bare `return;`) returns *undefined*;
  `isNull(noReturn())` → `YES`, stringifies to empty. (e8, e10)

---

## 4. Design

### 4.1 Function value representation

Extend `cfvariant::Function` (currently a bare text handle) to carry a real
callable. Options considered:
- **Store a JIT function pointer + metadata** (recommended). Each UDF/closure
  compiles to its own LLVM function; the `Function` value holds its address,
  param metadata and captured-scope pointer.
- Reject: an interpreter path would violate "cffunctions compiled as direct
  calls in the LLVM JIT, avoiding dynamic lookup/searches".

Proposed `Function` payload (new fields on `cfvariant`, only used when
`m_type == Function`):

```
struct UDFInfo {
    void *fn;                      // JIT'd entry point (see 4.3 signature)
    string name;                   // "" for anonymous closures
    vector<Param> params;          // name, default (nullable ExprAST/const), declared type
    string returnType;             // "", "void", "any", "numeric", ...
    bool isClosure;                // anonymous function(){} vs named function foo(){}
    cfvariant *capturedScope;      // enclosing scope instance (closures), or parent variables
};
```

`IsClosure` returns `isClosure`; `IsCustomFunction` returns true for all UDF
values (named or anonymous). Built-in method-handle `Function` values (bare
`#abs#`) keep the existing text-handle behavior and are distinguishable by
`fn == nullptr`.

### 4.2 Registration (compile-time, because functions are hoisted)

CF hoists UDFs across the whole template and stores them in `variables`. The
compiler therefore:
1. Scans the template for function declarations before emitting page code.
2. Compiles each declared function body into its own LLVM function.
3. Emits, at the start of the template's `main`, a loop that registers every
   function into the `variables` scope under its upper-cased name
   (`cfvariant_assign(variables, "ADDFN", functionValue)`), matching
   `StructKeyExists(variables, name) == YES`.
4. Enforces CF's compile-time rules at that scan:
   - name collides with a built-in → `InvalidUDFNameException` message;
   - name declared twice → `StaticDuplicateFunctionDefinitionException` message;
   - illegal characters → `IllegalUDFNameException` message.

A function declared inside another function's body is compiled as a closure
bound to that function's local scope (see 4.4) rather than a page-level UDF.

### 4.3 Call dispatch (the runtime function-name check)

`name(args)` compiles as follows in `CompileExprAST`/`compile_script_expr_token`:

- **Name is a compile-time-known built-in** → unchanged direct/native call.
  (UDFs can never shadow built-ins, so this is safe.)
- **Name is NOT a built-in** → emit a runtime helper
  `cfml::cfvariant_call_function(out, cgi, ..., variables, name, args, argc)`
  extended to:
  1. Resolve `name` as a variable (all scopes, as `lookupVarWritable` does).
     If it is a callable `Function` value (`fn != nullptr`), invoke it.
  2. Otherwise fall through to the existing built-in string dispatch.
  3. Otherwise throw "Variable {NAME} is undefined." (matching CF, 3.2).

Because the name was not a built-in, step 2 is a no-op for this path; it is kept
so `cfvariant_call_function` remains the universal dispatcher for the
interpreter/evaluate path too. A closure stored in a variable is found by the
same variable lookup, so `f(1,2)` and `s.fn(1)` need no extra compile-time
handling beyond emitting the runtime check for non-builtin callee names.

`callCallback` (cf8.cpp:1908) and the `Array*`/`Struct*` callback functions
must gain a branch: when the callback argument is a `Function` value with
`fn != nullptr`, invoke it directly (see 4.5) instead of
`evaluateExpr(callbackName + "(...)")`.

### 4.4 Compiling a function body (LLVM shape)

Every UDF/closure compiles to one LLVM function with a fixed signature modeled
on `template_fn`, plus a captured scope:

```
cfvariant *udf_main(string *out, void *cgi, void *server, void *cookie,
                    void *application, void *session, void *url, void *form,
                    cfvariant *parentScope, cfvariant **args, int argc)
```

Inside:
- Allocate a fresh `variables`/local scope struct; `var` declarations live here.
- Bind params: positional args fill param slots in order; missing params get
  their default value evaluated (if any) or stay undefined (`cfvariant::Null` /
  NotSet → "Variable A is undefined." on use); extra args are kept only in
  `arguments`.
- Build the `arguments` struct: upper-cased param-name keys + numeric indices
  1..N for every passed arg.
- Declared param/return types: coerce on entry / on `return`; a non-convertible
  value throws the matching `InvalidArgumentTypeException` /
  `InvalidReturnTypeException` message; `void` + return value throws
  `IllegalReturnException`.
- Unqualified reads walk local `var` scope then `parentScope`; unqualified
  writes target `parentScope` (CF gotcha, 3.5). `var` writes target local scope.
- The body is compiled by reusing `compile_token_list`/`compile_script_expression`
  with `return` handled via a return-value slot (the body's `return expr`
  stores into it and branches to a common exit block).
- A closure captures `parentScope` = the enclosing scope instance at creation.

### 4.5 Closure expression parsing (BUGS.md #1 fix)

In `parseTokensToAST` (src/compiler.cpp:762), when a `TextParser_cfml_Function`
token's extracted name is exactly `function` (case-insensitive) and it is
followed by a `{ ... }` CodeBlock token, build a new `ExprAST` node
`Closure` instead of a `FuncCall`:
- parameters from the `( ... )` children,
- body = the CodeBlock token (kept as raw tokens for later compilation).
`CompileExprAST` emits runtime code that constructs a `Function` value holding
the closure's JIT address and the current scope instance as `capturedScope`.

`mergeObjectMembers` and the existing `function` keyword handling must also be
audited so `function`/closure bodies inside expressions are consumed and the
"bare parameter reference" regression cannot reappear.

### 4.6 Scope-parameter threading

The JIT `main` currently receives `(cgi, server, cookie, application, session,
url, form, variables)`. `udf_main` additionally receives `parentScope` and the
arg array. All scope-dependent helpers (`cfvariant_bare_identifier`,
`cfvariant_assign`, `evaluateExpr`, `cfvariant_call_function`, callback
helpers) already take explicit scope pointers, so they work unchanged inside a
UDF once the UDF's local scope is passed as `variables`.

---

## 5. Edge cases / open questions

- **`var` scoping inside loops/if in script** — CF `var` is function-scoped;
  this compiler treats `var`-like declarations how? (TODO: check current
  handling; cfscript `var` statements are currently stripped in
  `compile_script_statement`.)
- **Function expressions in `<cfset>` / tag expression context** (`<cfset f =
  function(x){...}>`) — should work via the same closure path; verify CF.
- **Recursive UDF via its own name** — the UDF must see itself; since it is
  registered in `variables` before any call, a self-reference resolves normally.
- **`arguments` for page-level (template) code** — CF: `arguments` is undefined
  at page level ("Variable ARGUMENTS is undefined."); only functions define it.
- **Empty-name / illegal-char UDF names** — enforce `IllegalUDFNameException`.
- **`return null;`** — CF 2021 treats `null` as an undefined variable; our
  engine has a `null` literal. Decide: follow CF (error) or keep our extension.
- **Component (CFC) functions** are out of scope for this task.

---

## 6. Implementation plan (proposed order)

1. Parse closure expressions in `parseTokensToAST` (new `Closure` AST node) so
   BUGS.md #1 is fixed for the parse level; add cfm tests under
   `tests/cfm/` (e.g. `udf_closure_expr_test.cfm`) and unit tests.
2. Extend `cfvariant::Function` with the `UDFInfo` payload + create/invoke
   helpers; add `IsClosure`/`IsCustomFunction` wiring.
3. Compile a UDF body as an LLVM function (scopes, params, defaults,
   `arguments`, return slot, type coercion checks).
4. Register page-level function declarations at template start; enforce
   CF's compile-time name rules.
5. Runtime dispatch: extend `cfvariant_call_function` + non-builtin FuncCall
   compile path to resolve and invoke Function values; "Variable {NAME} is
   undefined." fallback.
6. Closures as values: emit closure construction at the `Closure` AST node;
   capture scope instance; per-invocation state.
7. Callback integration: `arrayMap`/`arrayFilter`/`arrayEach`/`arrayReduce`/
   `structEach`/`structMap`/etc. invoke Function values directly.
8. Full test pass: `./tests/verify_with_coldfusion.py` (single + full + `--exact`)
   with new cfm fixtures exercising every case in section 3; unit tests for
   corner cases; update BUGS.md (remove item #1 when fully fixed), PROGRESS.md,
   README.md.

### 6.1 Implementation status

All of steps 1–8 are done (callbacks: `arrayMap`, `arraySort` and the
`evaluateExpr`/`callCallback` paths invoke Function values directly;
`arrayFilter`/`arrayEach`/`arrayReduce`/`structEach`/etc. remain unimplemented
as separate features, but any callback that reaches `callCallback` handles a
closure). Key implementation notes:

- `UDFInfo` (cfvariant.h) carries the JIT entry pointer, name, closure flag and
  the captured scope. Built-in method handles keep the old text-only
  `Function` value (m_udf == null) so `#pi#` still renders a CFPageMethod handle.
- UDF bodies compile to their own LLVM functions with the 11-arg signature
  `(out, cgi..form, parentScope, args, argc)`; the prologue creates the local
  scope, pushes the `cf_udf_begin` context, marks local names (params + `var` +
  nested functions + `ARGUMENTS`), binds/coerces params (defaults compiled
  inline), builds `arguments`, registers nested functions, then compiles the
  body with a `return` slot + shared exit block. `g_returnCtx` (saved/restored
  around nested compiles) makes `return` work.
- Scoping: `lookupVarWritable` walks the passed local scope, then the
  thread-local `g_udfCtx` parent chain, then the fixed scopes;
  `cfvariant_assign` routes unqualified writes to the captured parent scope
  unless the name is a marked local — matching CF's UDF scope leak.
- Page-level UDFs are collected before code generation, compiled, and
  registered into `variables` at `main` entry (hoisting). CF's compile-time
  name rules (builtin collision, duplicate, illegal chars) are enforced.
- Known divergence (pre-existing, unrelated): a template ending with
  `<cfscript>` and no trailing newline emits a trailing space CF does not
  (see BUGS.md #1).

## 7. Verification commands

- Build: `./build.sh` / `./build.sh --unit-tests`
- Run a snippet: `./tmp/run_cf.sh tmp/udf_probes/eNN.cfm`
- Verify vs CF: `RDS_HOST=... ./tests/verify_with_coldfusion.py --dir tests/cfm/<new>.cfm`
- Grammar sanity: `textparser <file.cfm> --definition definitions/cfml_definition.json`

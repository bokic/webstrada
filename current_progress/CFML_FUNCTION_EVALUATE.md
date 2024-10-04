# Research: EVALUATE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`
- Implemented: `2026-08-05` (see `tests/cfm/evaluate.cfm`, byte-matched against CF 2025)

## Current state

- Runtime: `cfml::cf_evaluate(string &out, const cfvariant **args, int arg_count,
  void *cgi, ...)` in `src/cf8.cpp` — converts each argument to a string and runs
  it through the runtime expression evaluator `evaluateExpr` (the same engine used
  for `#...#` interpolation and callback dispatch), left to right, returning the
  rightmost result. Zero arguments throws the parameter-validation error.
- Compiler: `EVALUATE` is compiled by a dedicated native JIT handler in
  `src/compiler.cpp` that compiles each argument, builds a `cfvariant*` array, and
  calls `cf_evaluate` directly with the page output buffer plus the 8 live scope
  pointers (no dynamic lookup). It was removed from the zero-arg not-implemented list.
- Interpreter: `evaluateExpr`'s function-call dispatch also handles `EVALUATE`
  (so `Evaluate("evaluate('1+1')")` — a nested evaluate inside an evaluated string
  — works).
- Symbol registered at `src/compiler.cpp` (AddSymbol `cf_evaluate`).

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cf8.cpp` `cfml::cf_evaluate` |
| Compiler wiring | ✅ Native JIT handler (varargs + scopes + out) | `src/compiler.cpp` |
| Interpreter dispatch | ✅ `EVALUATE` case in `evaluateExpr` | `src/cf8.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/evaluate.cfm` (tag + script), verified byte-for-byte against CF 2025 | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_EVALUATE.md` |

## Behavior verified against Adobe ColdFusion 2025 (RDS host)

`Evaluate` evaluates one or more string expressions dynamically, left to right,
and returns the rightmost result. Verified cases:

- `Evaluate("1+1")` → `2`; `Evaluate("a + b")` (a=5, b=2) → `7`; `Evaluate("a")` → `5`.
- `Evaluate("#a# + #b#")` → `7` (`#…#` interpolation happens at compile time of the
  call-site string literal; the interpolated string is then evaluated).
- `Evaluate("a", "b")` → `2` (evaluates each arg in turn, returns the last).
- `Evaluate("de(a)")` → `"5"`; `Evaluate(de("a"))` → `a` (de wraps the string in quotes).
- Boolean rendering follows the same rules as evaluating the expression directly:
  `Evaluate("a EQ 5")` → `YES`, `Evaluate("b")` (b=true) → `true`,
  `Evaluate("a EQ 5 AND b")` → `true`, `Evaluate("a EQ 6 AND t")` → `NO`.
- `Evaluate("")` → empty string (no error); `Evaluate()` → compile-time
  parameter-validation error (not catchable), matching CF.
- Array indexing / query columns work: `Evaluate("arr[2]")` → `20`,
  `Evaluate("q.id")` → first row's cell (empty for a 0-row query).
- Nested `Evaluate("evaluate('1+1')")` → `2`.

## Parameter passing

ColdFusion passes simple-value arguments by value; Evaluate takes only simple
string arguments, so the caller's variables are never mutated by the call itself.

## Dependency

Depends on the runtime expression evaluator `evaluateExpr` (already present in
`src/cf8.cpp`); no secondary JIT compilation is needed.

## Ease of implementation

Was initially rated Hard (self-hosting compiler); the existing `evaluateExpr`
runtime expression engine made the actual implementation straightforward.

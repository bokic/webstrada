# Research: RANDRANGE cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120`
- Timestamp: `2026-08-03 20:08:32 UTC`
- Implemented: `2026-08-06` (see `tests/cfm/randrange_test.cfm`, byte-matched against CF 2025)

## Current state

- Runtime: `cfml::cf_randrange(const cfvariant *number1, const cfvariant *number2, const cfvariant *algorithm)` in `src/cf8.cpp` — returns a random integer in the inclusive range `[min, max]`, swapping the bounds when `number1 > number2` (CF behavior, verified on the RDS host). The optional `algorithm` argument is validated (default SHA1PRNG); an unsupported name throws CF's "The X algorithm is not supported by the Security Provider you have chosen." (Expression, catchable). Randomness comes from the same `rand()` generator as `Rand`/`Randomize`.
- Compiler: `RANDRANGE` has a dedicated native JIT handler in `src/compiler.cpp` that compiles the 2–3 arguments and calls `cf_randrange` directly (no dynamic lookup). It was removed from the zero-arg not-implemented list.
- Interpreter: `evaluateExpr`'s function-call dispatch handles `RANDRANGE`.
- Symbol registered at `src/compiler.cpp` (AddSymbol `cf_randrange`).

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cf8.cpp` `cfml::cf_randrange` |
| Compiler wiring | ✅ Native JIT handler (2–3 args) | `src/compiler.cpp` |
| Interpreter dispatch | ✅ `RANDRANGE` case in `evaluateExpr` | `src/cf8.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/randrange_test.cfm` + `JitExpressionTest.RandRange*` unit tests, verified against CF 2025 | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_RANDRANGE.md` |

## What RANDRANGE does at the low C level

Return a random integer in the inclusive range `[number1, number2]`. The bounds are swapped when `number1 > number2` (verified: `RandRange(5,3)` yields values in [3,5], `RandRange(7,5)` in [5,7]); equal bounds always return that value. The `algorithm` argument selects the generator.

- Arg 1: `number1` (number, required).
- Arg 2: `number2` (number, required).
- Arg 3: `algorithm` (string, optional, default SHA1PRNG).

## Parameter passing

Simple-value arguments are passed by value; the function takes only simple arguments.

## Proposed compiled form

```cpp
cfvariant *cf_randrange(const cfvariant *a1, const cfvariant *a2, const cfvariant *a3);
```

## Dependency

Uses the existing `rand()`/`RAND_MAX` generator (shared with `cf_rand`/`cf_randomize`).

## Ease of implementation

Easy. `rand()` over the inclusive range with CF's bound swap.

## Known divergence

None. The per-thread caching was fixed: the algorithm is validated only on the first `Rand`/`RandRange` call of a thread (a `thread_local` flag mirrors CF's never-cleared per-thread SecureRandom cache), and `Randomize` validates on every call like CF's reseeding 2-arg overload. Verified against CF 2025 in `tests/cfm/rand_algorithm_caching_test.cfm` (was BUGS.md).

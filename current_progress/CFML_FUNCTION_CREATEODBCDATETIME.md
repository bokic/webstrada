# Research: CREATEODBCDATETIME cffunction implementation notes

- Git commit: `6881a2511a9fe67b582286462f66573bb8b5e16b`
- Timestamp: `2026-08-03 20:19:57 UTC`
- Implemented: `2026-08-05` (see `tests/cfm/createodbcdatetime.cfm`, byte-matched against CF 2025)

## Current state

- Runtime: `cfml::cf_createodbcdatetime(const cfvariant *date)` in `src/cf8.cpp` —
  converts the input (DateTime, serial Number/Long/Float, or a parseable date
  string) to days via `getDaysFromVariant` and returns a `DateTime` cfvariant.
  Invalid input throws CF's exact runtime `Expression` error
  `The value <input> cannot be converted to a date.`.
- Compiler: `CREATEODBCDATETIME` is compiled by the generic 1-argument native
  JIT handler (`src/compiler.cpp`, same list as `ISDATE`/`YEAR`/...) — removed
  from the not-implemented list, symbol `cf_createodbcdatetime` registered.
- Interpreter: `evaluateExpr`'s function-call dispatch handles `CREATEODBCDATETIME`
  (so it works inside an `Evaluate` string too).
- Fixed while verifying: `compareVariants` (src/cf8.cpp) did not compare
  `DateTime` values at all (`d EQ d` was NO). It now compares two DateTimes by
  serial value, a DateTime against a parseable date string, and GT/LT on a
  DateTime uses its serial value (a non-date string → 0), matching CF 2025
  (`CreateDateTime(..) EQ CreateDateTime(..)` → YES, `.. EQ "2024-05-15 13:45:30"` → YES,
  `.. EQ 45427` → NO, `.. GT "bad-date"` → YES).

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ Implemented | `src/cf8.cpp` `cfml::cf_createodbcdatetime` |
| Compiler wiring | ✅ 1-arg JIT handler | `src/compiler.cpp` |
| Interpreter dispatch | ✅ `CREATEODBCDATETIME` case in `evaluateExpr` | `src/cf8.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/createodbcdatetime.cfm`, verified byte-for-byte against CF 2025 | — |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` (ODBC) | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_CREATEODBCDATETIME.md` |

## Behavior verified against Adobe ColdFusion 2025 (RDS host)

`CreateODBCDateTime` creates an ODBC timestamp from a date value. The returned
value renders as `{ts 'yyyy-mm-dd hh:nn:ss'}` and behaves like a plain date.

- `CreateODBCDateTime(CreateDateTime(2024,5,15,13,45,30))` → `{ts '2024-05-15 13:45:30'}`.
- Accepts a date string: `"2024-05-15 13:45:30"` (kept), `"2024-05-15"` → midnight.
- Accepts a serial number: `CreateODBCDateTime(2004)` → `{ts '1905-06-26 00:00:00'}`
  (2004 days since 1899-12-30).
- `DateTimeFormat`/`DateFormat`/`TimeFormat`/`Year`/`Month`/`Day`/`Hour`/`Minute`/
  `Second`/`DateDiff`/`DateCompare`/`IsDate`/`IsNumericDate` all work on the value.
- `CreateODBCDateTime(d) EQ d` → YES (and EQ/NEQ/GT/LT against plain dates and
  date strings match CF).
- Invalid input (`"bad-date"`) throws catchable `Expression` error
  `The value bad-date cannot be converted to a date.`.
- `CreateODBCDateTime()` with no args is a compile-time parameter-validation error.

## Parameter passing

ColdFusion passes simple-value arguments by value; the caller's variables are
never mutated by the call.

## Dependency

None beyond the existing `getDaysFromVariant` date parser.

## Ease of implementation

Easy. A DateTime-value wrapper. `CreateODBCDate`/`CreateODBCTime` (which render
`{d '...'}`/`{t '...'}`) remain unimplemented stubs.

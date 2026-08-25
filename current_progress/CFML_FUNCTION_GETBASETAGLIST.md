# Research: GETBASETAGLIST cffunction implementation notes

- Git commit: `316718b50c043a37d37dd2b9373ed4341a43b120` (stub), rewritten `2026-08-25`
- Timestamp: `2026-08-25`

## Current state

- Runtime: `cfml::cf_getbasetaglist()` in `src/core/core_misc.cpp` — returns the
  comma-delimited list of currently-executing base tags, innermost first.
- Compiler: `GETBASETAGLIST` is compiled as a zero-arg runtime call
  (`cf_getbasetaglist`); the interp dispatch is at `src/core/core_interp.cpp`
  (`GETBASETAGLIST` → `cfml::cf_getbasetaglist`).
- Symbol registered in `src/codegen/llvm_compiler.cpp`.

## Implemented: 100% (byte-verified vs CF 2025)

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ `cfml::cf_getbasetaglist()` | `src/core/core_misc.cpp` |
| Compiler wiring | ✅ JIT symbol + interp dispatch | `src/core/core_misc.cpp`, `src/core/core_interp.cpp` |
| Tag support | N/A (function only) | — |
| Tests | ✅ `tests/cfm/getbasetaglist_{current,ctx,branch2,parent_cfoutput,root}_test.cfm` + `custom_tag_basetaglist_test.cfm` + `custom_tag_nested_test.cfm`, all byte-verified vs CF 2025 | — |
| Unit tests | ✅ `ComponentTest.GetBaseTagListIncludesCfdoutputOnlyInsideCfdoutput` + `ComponentTest.GetBaseTagListParentCfdoutputOrdering` | `tests/tests.cpp` |
| Tracker status | ✅ `PROGRESS.md` (✅ Yes), removed from `UNIMPLEMENTED_FUNCTIONS.md` | — |
| Docs/spec | ✅ Spec reference exists | `cfml_docs/CFML_FUNCTION_GETBASETAGLIST.md` |

## Behavior (verified on Adobe ColdFusion 2025)

GetBaseTagList returns the currently-executing "base tag" stack from innermost to
outermost, joined by commas. The stack has one entry per executing tag context:

- A `<cfoutput>` block pushes `CFOUTPUT` while it runs. So inside a `<cfoutput>`
  the innermost entry is `CFOUTPUT`; outside one the innermost entry is the
  current custom tag.
- Each active custom tag contributes its public name (`CF_<NAME>` for
  imported/prefixed tags, `CF_<filename>` for `<cfmodule>` tags), innermost first.

Observed cases (CF 2025):

| Context | GetBaseTagList |
|---|---|
| Root template, outside `<cfoutput>` | `""` |
| Root template, inside `<cfoutput>` | `CFOUTPUT` |
| Custom tag, outside its own `<cfoutput>` | `CF_XXX` |
| Custom tag, inside its own `<cfoutput>` | `CFOUTPUT,CF_XXX` |
| Custom tag invoked from a parent `<cfoutput>`, calling outside its own `<cfoutput>` | `CF_XXX,CFOUTPUT` |

Implementation: a `thread_local g_baseTagStack` (declared `core_internal.h`,
defined `src/cftags/tag_custom.cpp`) is maintained in parallel with the custom
tag stack. `cf_custom_tag_begin` pushes the tag's public name, `cf_custom_tag_finish`
pops it (also popping any unbalanced `<cfoutput>` markers left by an exception),
and the compiler emits `cf_cfoutput_begin` / `cf_cfoutput_end` around every
`<cfoutput>` body (both the plain and `query=` forms). `GetBaseTagData` keeps
using `g_customTagStack` (no markers). Both stacks are cleared per request in
`scope_begin` via `cfml::custom_tag_stack_clear()`.

This fixed Mango Blog's `tags/mango/Posts.cfm` "Variable POSTS is undefined."
error: the tag does `listdeleteat(getbasetaglist(),1)` then
`listfindnocase(...,"cf_posts")` to detect its context. The old engine prepended
`CFOUTPUT` unconditionally, so after the delete the wrong list remained and the
`parent` branch (with a commented-out `posts` assignment) ran.

## Dependency

The base-tag stack and the `<cfoutput>` begin/end markers.

## Ease of implementation

Hard. Shares the custom-tag stack infrastructure with `GetBaseTagData`.

# Research: CFERROR cftag implementation notes

- Git commit: `6b236fb` (cferror work, 2026-08-09)
- Timestamp: `2026-08-09 UTC`

## Current state

- Implemented and byte-verified against CF 2025 on the RDS host
  (`tests/cfm/cferror_*_test.cfm` + `tests/cfm/include_lib/cferror_*_page.cfm`, all green).
- Runtime: `src/cftags/tag_cferror.cpp` (`cf_cferror_register`, `cf_cferror_handle`, `cferror_reset`).
- Compiler: `supported_tags` + a dedicated `<cferror>` branch (compile-time attribute
  validation, emits a `cf_cferror_register` call) in `src/codegen/llvm_codegen.cpp`.
- PROGRESS.md: `cferror` is `✅ Yes`; removed from UNIMPLEMENTED_TAGS.md.

## Implemented: 100%

## Status checklist

| Area | Status | Location |
|------|--------|----------|
| Runtime | ✅ `cf_cferror_register` / `cf_cferror_handle` / `cferror_reset` | `src/cftags/tag_cferror.cpp` |
| Compiler wiring | ✅ `supported_tags` + cferror branch | `src/codegen/llvm_codegen.cpp` |
| Worker wiring | ✅ inner dispatch + 404 for TemplateNotFoundException | `src/worker.cpp` |
| Tag support | ✅ | — |
| Tests | ✅ 10 verify tests + 4 include_lib pages, all byte-identical with CF 2025 | `tests/cfm/cferror_*` |
| Tracker status | ✅ `PROGRESS.md` ✅ Yes | — |
| Docs/spec | ✅ `cfml_docs/CFML_TAG_CFERROR.md` | — |

## Verified CF 2025 semantics (all live-probed on RDS 192.168.100.10:8500)

### ErrorTag.doStartTag (decoded from chf20250011.jar ErrorTag)

1. Template resolution FIRST — a bad template beats a bad type value:
   - `dir = pagePath.substring(0, pagePath.lastIndexOf(separator) + 1)` — the
     trailing separator is KEPT (both `/` and `\`).
   - candidate1 = dir + template value; if the file exists → use it.
   - else if value starts with `/` or `\` → candidate2 = webRoot + value.
   - else → throw `MissingInclude` "Error attempting to resolve the template {value}."
     (the RAW attribute value, verified for both relative and absolute) /
     "The template could not be found.". Catchable, type "MissingInclude".
2. Type dispatch: `validation` → setValidationErrorHandler; `exception` → handler
   registration; `request` → setRequestErrorHandler; anything else (including
   empty) → `InvalidTagAttributeException` → runtime-catchable, type "Application",
   message "Attribute validation error for tag CFERROR.", detail
   "The value of the attribute type, which is currently {value}, is invalid."
   (empty value renders as `''`).
3. Unknown attributes / missing `type`+`template` are TRANSLATION-TIME errors
   (the page never runs, nothing catches them): message "Attribute validation
   error for tag CFERROR.", detail "It does not allow the attribute(s) X. The
   valid attribute(s) are EXCEPTION,MAILTO,TEMPLATE,TYPE." / "It requires the
   attribute(s): TEMPLATE,TYPE." — attribute lists are sorted, comma-separated
   (no space).

### exception-class table (ErrorTag.doStartTag)

application→ApplicationException, database→DatabaseException, lock→LockException,
object→ObjectException, missinginclude→MissingIncludeException,
template→TemplateException, security→AccessControlException (java.security),
expression→java.lang.RuntimeException, any→**java.lang.Throwable**,
anything else → try `ClassLoader.loadClass(name)`; on failure →
`CustomException` with the name as the handler name (name only stored when
non-empty). Absent `exception` attribute defaults to "any".

### addExceptionHandler (FusionContext)

- Custom handlers (`CustomException` class): replace an existing custom handler
  only when both are unnamed or the names match case-insensitively; always
  APPEND at the end otherwise.
- Built-in handlers: replace on exact class equality; insert before the first
  existing handler whose class is an ancestor of the new handler's class
  (diffClassTypes != -1); otherwise append.

### matchExceptionHandler (FusionContext.handleException)

- Skip handlers where `diffClassTypes(cls, handlerClass) == INT_MAX`
  (handler class not an ancestor of the thrown class).
- `cls` = "CustomException" for custom exceptions, else the thrown type's class.
- Custom exceptions: remember the "any" (Throwable-class) handler as fallback;
  skip UNNAMED handlers (CF: handler.exception == null); match names
  case-insensitively; the fallback runs only when nothing matched.
- Non-custom exceptions: first class-distance match wins, fallback unused.

### handleException flow (cf_cferror_handle)

- once-per-request marker (biscuit): only the first exception per request is
  dispatched; `cferror_reset()` at request start.
- `m_missingTemplate` (TemplateNotFoundException) → return 0 (built-in 404;
  cferror never runs).
- exception handler matched → run as an include sharing all scopes; success → 1
  (status stays 200).
- handler threw a non-abort exception → request handler with the NEW exception
  (handlers not re-run; a copy is made because the original is on the unwind
  stack — the `&newEx` dangling-pointer bug).
- `<cfexit>` / abort inside a handler → 1.
- request handler success → status 500 (CF: EnableHTTPStatus).
- missing request-handler template file → return 0 (built-in error page).

### error / cferror struct (CfErrorWrapper + ExceptionScope)

- Wrapper keys (verified with structKeyList): Suppressed, GeneratedContent,
  Mailto, RootCause, RemoteAddress, StackTrace, QueryString, HTTPReferer,
  Template, Message, Diagnostics, DateTime, Browser, Type, TagContext — 15 keys.
- Wrapper TYPE is always "coldfusion.runtime.CfErrorWrapper".
- ExceptionScope pseudo-keys (ExceptionScope.get fallback) resolve to "" for the
  wrapper: detail, errorcode, extendedinfo, exceptions → structKeyExists is TRUE
  and reads are empty. The type-dependent pseudo-keys (nativeerrorcode, sqlstate,
  errnumber, missingfilename, lockname, lockoperation) are NOT present → the
  undefined-variable error, like CF.
- ROOTCAUSE keys (verified): extended_info, Suppressed, code, ExtendedInfo,
  StackTrace, Detail, Message, ErrorCode, Type, TagContext — 10 keys, with the
  REAL throw values (rootcause.detail/errorcode/extendedinfo work).
- DATETIME format: java Date.toString "EEE MMM dd HH:mm:ss zzz yyyy"
  (strftime `%a %b %d %H:%M:%S %Z %Y`).
- cfcatch is a NeoException-backed ExceptionScope: structKeyList shows the 9
  root-cause-shaped keys (Suppressed, code, ExtendedInfo, StackTrace, Detail,
  Message, ErrorCode, Type, TagContext), detail/errorcode/extendedinfo read the
  real values, nativeerrorcode etc. are undefined (probed).

### Request handler (type="request")

- The template is read as plain text (UTF-8 BOM stripped) and the buffer is
  cleared before writing it.
- 12 placeholders replaced case-insensitively, ALL occurrences, unknown
  placeholders left literal: #ERROR.GENERATEDCONTENT#, #ERROR.DIAGNOSTICS#,
  #ERROR.MAILTO#, #ERROR.DATETIME#, #ERROR.BROWSER#, #ERROR.REMOTEADDRESS#,
  #ERROR.HTTPREFERER#, #ERROR.QUERYSTRING#, #ERROR.TEMPLATE#, #ERROR.ROOTCAUSE.TYPE#,
  #ERROR.ROOTCAUSE.MESSAGE#, #ERROR.ROOTCAUSE.DETAIL#.
- On success the request completes with HTTP 500 (verified live).

### Engine implementation notes

- `exceptionClassForAttr` returns nullptr for unknown names (a non-null "" would
  have been a truthy pointer → blank-class handler).
- Class hierarchy gained `java.lang.Throwable` as the root (ErrorTag "any"
  registers Throwable, not Exception). cf_eh_class_distance stops at Throwable.
- Template resolution uses `include_context()->currentPath` (the executing
  page's dir) and `webRoot`.
- Worker: run_template stats the request page first so a missing page throws
  CF's TemplateNotFoundException ("Template not found!" / "Error Loading template
  {path}") with m_missingTemplate instead of the parser's "Parsing error!" —
  cferror never handles it and the daemon answers 404.
- The newline-after-`</cfoutput>` → space whitespace rule matches CF exactly
  (verified: ws1/ws4 probes, byte-for-byte).

## Known divergences (see BUGS.md)

- Diagnostics lacks CF's " <br>The error occurred on line N." suffix.
- Browser / RemoteAddress / HTTPReferer / QueryString / StackTrace are empty
  (CF fills them from the request); Suppressed is `false` (CF: java array
  toString).
- The ExceptionScope pseudo-keys are real map keys on our side, so
  structKeyList(error) shows 19 entries instead of CF's 15.
- type="validation" handlers are stored but never dispatched (no form
  validation in this engine).

## Not covered

- Site-wide missing_template / missing "request" handler → built-in error page.
- onError in Application.cfc has precedence over cferror (existing behavior).
- The request-handler HTTP-500 path cannot be automated by the verify harness
  (it treats HTTP errors as test errors) — verified manually via curl + CLI.

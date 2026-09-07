Next areas to work on:

* Implement all cfusion functions per UNIMPLEMENTED_FUNCTIONS.md
* Implement all cfusion tags per UNIMPLEMENTED_TAGS.md

* Make sure we support all cftags with all combinations, aswell all cffunctions
* Make sure we catch at compile-time all illegal cftags/cffunctions. as well their illegal usages scnarios.
* Implement DB backends: Oracle, Microsoft SQL Server.
* ODBC (via unixodbc)
* Spreadsheet (via xlnt)
* Java objects (using jnipp)
* .NET objects (using libmono)
* SOAP (using libcurl, libxml2, or gSOAP)
* Web Services (using libcurl, libxml2)
* ORM (Custom implementation)

* Optimizations:
* Add a per-function cache for first-level `cfvariant` member accesses:
  * At compile time, identify all eligible accesses.
  * Allocate a fixed-size pointer array in the function frame.
  * Assign each eligible access a compile-time slot index.
  * Initialize all slots to `nullptr`.
  * On the first access, resolve the member normally and cache its pointer.
  * On subsequent accesses, reuse the cached pointer.
  * After any external or potentially mutating call, clear the function's cache because the callee may delete, replace, or modify struct members.
  * Retain generation/owner validation so local deletions, clears, scope replacement, recursion, and aliasing cannot produce stale pointers.
* Add compile-time-sized temporary storage for each template and function:
  * Calculate the required number of temporary slots during code generation.
  * Allocate independent temporary storage for every invocation.
  * Initialize all slots as empty/`Null`.
  * Reuse slots for expression temporaries whose lifetimes do not overlap.
  * Track active temporary slots per cleanup region, including loop iterations and nested function/exception regions.
  * Clear released slots when leaving a region, on early exits, and during exception unwinding.
  * Temporary slots must never escape their owning template/function invocation or cleanup region.
  * Move values out of temporary slots when the source slot's lifetime ends; otherwise copy them before assigning, returning, or retaining them after the slot's region is cleared.

* git tag 0.9

* Project deslopization

* git tag 1.0

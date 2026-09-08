Next areas to work on:

## ColdFusion 2025 compatibility backlog

This is the technical follow-up to the overview in `README.md`. Keep exact implementation status in `PROGRESS.md`, `UNIMPLEMENTED_TAGS.md`, and `UNIMPLEMENTED_FUNCTIONS.md`.

### 2025 language and runtime features

* Implement null-coalescing (`??`) and safe-navigation (`?.`) operators.
* Implement lambda (`=>`) expressions and CFML spread (`...`) syntax for arguments, arrays, and structs.
* Implement function-parameter destructuring, multiple destructuring/mixed parameters, and related destructuring edge cases.
* Implement multi-assignment operators, trailing commas, and multiple exception arguments in catch clauses.
* Implement dynamic CFML property expressions.
* Implement 2025 Query-of-Queries modulus/bitwise operators and the Update 8 QoQ precedence rules.
* Implement `<cfquery cacheMaxIdleTime="...">` semantics and interaction with `cachedafter`/`cachedwithin`.
* Complete Update 8 language additions: native Sets and set operations, `asyncAllOf()`/`asyncAnyOf()` and CompletableFuture-style composition/timeouts, CFML callback adaptation to Java functional interfaces, member calls on literals, safer implicit arrays/structs in ternary/Elvis expressions, and the new/enhanced built-in functions.
* Add the Update 8 built-ins and changes not currently represented in the function inventory: `Struct.entries`, callback-capable `StructFind`, `ListFindLast`, `FileMismatch`, `ArrayFindLast`, `ArrayGetAt`, and `FileReadLines`; verify the Update 8 changes to `DirectoryCreate`, `queryNew`, `GetApplicationMetadata`, and XML `DOCTYPE` handling.

### 2025 feature APIs and services

* Implement the complete Spreadsheet API, including workbook read/write, cell/row/column formatting and metadata, hyperlinks, validation, page layout, and streaming spreadsheet APIs. CSV functions are already implemented.
* Implement server-side charting, `cfchartset`, SVG output, new chart types, themes, number formatting, markers, rules, and animation.
* Implement PDF/HTML-to-PDF/document/presentation/report tags and their associated functions.
* Implement `cfthread`, thread interruption/join/termination, and the corresponding async execution model.
* Implement WebSocket support and the 2025 HTTP streaming/SSE enhancements.
* Implement Java and .NET interoperability, including JavaCast, ReleaseComObject, DotNetToCFType, Java objects, and Java functional-interface integration.
* Implement ORM/entity/HQL support.
* Implement SOAP/web services, REST lifecycle helpers, gateways, Exchange, SharePoint, LDAP, and the remaining mail/network integrations.
* Implement SAML/OAuth authentication helpers and JWT signing/encryption helpers.
* Implement the remaining search, printer, K2, store-ACL/metadata, and security/integration functions listed in `UNIMPLEMENTED_FUNCTIONS.md`.

### ColdFusion 2025 Update 8

* Add native provider-neutral AI/LLM functions and configuration for OpenAI, Anthropic, Mistral, Gemini, and Ollama.
* Add MCP client and server support, tool discovery, and controlled tool invocation.
* Add vector-store integrations (InMemory, Pinecone, Milvus, Qdrant, Chroma), embeddings, `simpleRAG()`, agent ingestion, and document-service ingestion.
* Add passkey/WebAuthn/FIDO2 authentication, Argon2 password hashing, and AI guardrail validators.
* Add the runtime-facing AI services/trace telemetry needed for monitoring; IDE extension and PMT UI parity are separate product concerns.

### Scope and verification

* Preserve Adobe ColdFusion 2025 behavior and error messages; do not use Lucee behavior as the compatibility target.
* Add CFML fixtures with normal, boundary, invalid, and nested cases for each completed feature, parse them with `textparser <file> --definition definitions/cfml_definition.json`, and byte-verify against Adobe ColdFusion.
* Keep unsupported features as explicit runtime parser exceptions until implemented; do not silently accept a partial behavior.

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

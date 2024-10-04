Next areas to work on:
* Remove code comments
* Final check for sensitive informations before opensourcing the project
* Run all tests with asan
  * Done 2026-08-10 — see the item above. The request-wide temp-variant leaks are fixed.
* Admin/config page(mostly done 2026-08-11: server config file webstrada-config.json + config::initialize/save/reloadIfChanged; __configGet/__configSet/__configReset/__datasourceTest/__serverInfo compiler extensions; admin/api/*.cfm JSON endpoints; http-dev.py serves the built Angular app at /admin/ with SPA fallback; Dashboard + General Settings + Data Sources pages wired live. Still to do: Cache, Logs, Runtime, Users pages live data; API login/auth; request-stats shared across prefork workers; verify_with_coldfusion.py skip-list note for admin_extension_test.cfm)

* git tag 0.8
* open source the project
* generate/publish docker images(latest and per git tag)

* Fix the residual ASan leaks: OpenSSL legacy-provider state is unloaded at process exit; the ~7 KB scope-store residue in struct_scope_functions_test.cfm (see BUGS.md "Every request leaked memory")

* Implement all cfusion functions per UNIMPLEMENTED_TAGS.md
* Implement all cfusion tags per UNIMPLEMENTED_TAGS.md

* Make sure we support all cftags with all combinations, aswell all cffunctions
* Make sure we catch at compile-time all illegal cftags/cffunctions. as well their illegal usages scnarios.
* Implement DB backends: Oracle, Microsoft SQL Server (each is a new `DBDriver`/`DBConnection` registered in the abstract layer; SQLite, MySQL/MariaDB, PostgreSQL with their per-backend `<cfdbinfo>` metadata and `<cfstoredproc>`/`<cfprocparam>`/`<cfprocresult>` via CALL are done)
* ODBC (via unixodbc)
* Spreadsheet (via xlnt)
* Java objects (using jnipp)
* .NET objects (using libmono)
* SOAP (using libcurl, libxml2, or gSOAP)
* Web Services (using libcurl, libxml2)
* ORM (Custom implementation)

* Optimization
* implement local cfvariant key cache. check drop cache if code line has external calls
* implement prefefined array of temporaries per template, and function
* change all cffunctions so they will return void, and update return value by last cfvariant &out parameter

* git tag 0.9

* Project deslopization

* git tag 1.0

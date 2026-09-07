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

* Optimization
* implement local cfvariant key cache. check drop cache if code line has external calls
* implement prefefined array of temporaries per template, and function
* change all cffunctions so they will return void, and update return value by last cfvariant &out parameter

* git tag 0.9

* Project deslopization

* git tag 1.0

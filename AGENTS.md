Definitions:
* CFML is the ColdFusion Markup Language.
* `cftag` is tag defined in CFML(see in cfml_docs dir all CFML_TAG_\*.md files)
* `cffunction` is function defined in CFML(see in cfml_docs dir all CFML_FUNCTION_\*.md files)

Rules:
* Implement same behavior as Adobe ColdFusion, not open source variants like Lucee and other.
* If some unit test fail. Do not fix the test result by crippling the test; instead, fix the test itself to avoid future regressions. Change the unit test only if there is a bug in it.
* If some task is getting more complex compared to what was originally intended, stop trying to implement it, and explain to be the complexity and how you think it should be continued, so I can decide how if you can continue with your direction or I can give you better guidance.
* Run one task at a time, nothing else. If you find issues, append them to the proper file: bugs/behavioral divergences or deliberate limitations in this codebase → `BUGS.md`; bugs found *inside ColdFusion itself* (server/installation problems on the RDS host that block verification, CF quirks the engine reproduces) → `BUGS_CF.md`; cosmetic output/whitespace/display artifacts in our code that do not affect functional correctness → `BUGS_COSMETIC.md`.
* After every task implement unit tests with corner cases and edge conditions.
* After every task, update the PROGRESS.md, README.md, UNIMPLEMENTED_FUNCTIONS.md, and other md files to reflect the new implementation and found information.
* Always check if this file(or TODO.md) has been updated before making any changes. If it has, reread it and update your knowledge accordingly.
* Implement cffunctions as it's own function(do not generate embedded/inline one). All cffunctions need to be compiled as direct calls in the LLVM JIT compiler, avoiding dynamic lookup/searches.
* Every cffunction argument should be `cfvariant *`(or `const cfvariant *`) type. Return type should be `cfvariant *` or void.
* If you find something that currently(or at all) can't be implemented, implement plaun function that just throws runtime parser exception with coresponding message.
* If some task is not fully understandable, is not aligned with your knowledge, or not fully explained, ask user for clarification, stop with thinking and don't proceed with the task until it's clear.
* If you find issue that is inside textparser engine, just report it in details do not try to fix it.
* Once you fully fix an item in a BUGS file (BUGS.md / BUGS_CF.md / BUGS_COSMETIC.md), delete it from the list.
* If you fully fix an item in a BUGS file and found another issue(s), delete the item from the list and add the new issue(s) to the proper file.
* Create tmp dir in root of the project and use it for storing temporary files to skip access to `/tmp`.
* Test textparser engine with the following command `textparser <cfml_file>`. put cfml snippet in `<cfml_file>` and test it. Always point the tool at the project's authoritative grammar: `textparser <cfml_file> --definition definitions/cfml_definition.json` (the project's local copy is kept in sync with the system package at `/usr/share/textparser/definitions/`; when the system package is updated, re-sync it by copying the updated `cfml_definition.json` over the local copy and regenerating `definitions/cfml_definition.json.h` with `textparser_json2h.py`).

Knowledge:
* If needed read the PROGRESS.md file to see which cf tags/functions are already implemented, and update it when you implement a new one.
* List of all cf tags/function with description what they do are in cfml_docs dir.
* If you don't know how to completely implement something, you can consult the decompiled ColdFusion 2025 sources to see the authoritative engine behavior. The decompiled Java source is in `tmp/cfdecomp/src/cfusion/sources/` (produced with jadx from `cfusion.jar`). The distilled knowledge about scopes, templates, and variables is in `tmp/cf_jars/KNOWLEDGE.md` (see also `tmp/cf_jars/CF_FILES.md` for what is in the unpacked installer).

Build/Run/Test:
* Build: `./build.sh`
* Build with unit tests: `./build.sh --unit-tests`
* To see the generated CLang AST of a given CFML block: `echo "{some CFML block}" | PRINT_AST= ./bin/WebStrada-cli --stdin`
* To execute given CFML block, and see the output: `echo "{some CFML block}" | ./bin/WebStrada-cli --stdin`

Verification:
You can verify this project cfm execution output with local ColdFusion installation using our Python test suite:
* Run all tests: `RDS_HOST={COLDFUSION_HOST_IP} ./tests/verify_with_coldfusion.py`
* Run a single file: `RDS_HOST={COLDFUSION_HOST_IP} ./tests/verify_with_coldfusion.py --dir tests/cfm/cfif1.cfm`
* Run with strict exact match: `RDS_HOST={COLDFUSION_HOST_IP} ./tests/verify_with_coldfusion.py --exact`
* RDS_HOST might be already set in the environment.<[fim-middle]>
If RDS connection returns with an error, stop thinking and tell me to restart ColdFusion Server.

Unit tests:
* When working on a new cftag or cffunction always add cfm file to tests/cfm dir with as much possible variants of the feature and always test it's output with verify_with_coldfusion.py script. Don't mark task as complete until all tests passed.

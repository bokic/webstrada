<!---
  Compiler-extension functions (the reserved `__` prefix) — engine regression
  test. These are WebStrada extensions, NOT part of ColdFusion, so this file
  cannot be byte-verified against CF via verify_with_coldfusion.py (running it
  there produces "Variable __CONFIGGET is undefined").
  Run locally with: ./bin/WebStrada-cli tests/cfm/admin_extension_test.cfm
--->
<cfset __configSet({settings: {dsnDbDir: GetTempDirectory()}})>

<cfoutput>configGet charset=#__configGet().settings.defaultOutputCharset#
configGet whitespace=#__configGet().settings.enableWhitespaceManagement#</cfoutput>

<cfset r = __configSet({settings: {defaultOutputCharset: "ISO-8859-1", enableQueryLogging: true}})>
<cfoutput>configSet charset=#r.settings.defaultOutputCharset#
configSet queryLogging=#r.settings.enableQueryLogging#</cfoutput>

<!--- Unknown setting key is rejected (catchable runtime error) --->
<cftry>
<cfset __configSet({settings: {bogus: 1}})>unknownSetting=no error
<cfcatch><cfoutput>unknownSetting=#Left(cfcatch.message, 30)#</cfoutput></cfcatch>
</cftry>

<!--- Missing argument --->
<cftry>
<cfset __configSet()>missingArg=no error
<cfcatch><cfoutput>missingArg=#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- Named arguments are rejected --->
<cftry>
<cfset __configSet(settings = {})>namedArgs=no error
<cfcatch><cfoutput>namedArgs=#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- Non-numeric value for a numeric setting --->
<cftry>
<cfset __configSet({settings: {charsetDetectionMinConfidence: "abc"}})>nonNumeric=no error
<cfcatch><cfoutput>nonNumeric=#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- Unsupported datasource backend --->
<cftry>
<cfset __configSet({datasources: {appdb: {backend: "oracle"}}})>badBackend=no error
<cfcatch><cfoutput>badBackend=#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- Datasource upsert, password masking, delete --->
<cfset __configSet({datasources: {appdb: {backend: "sqlite", password: "secret"}}})>
<cfset cfg = __configGet()>
<cfset dsGet = cfg.datasources.appdb>
<cfoutput>dsPasswordMasked=#dsGet.password#</cfoutput>
<cfset __configSet({datasources: {appdb: {action: "delete"}}})>
<cfset cfg2 = __configGet()>
<cfoutput>dsDeleted=#StructKeyExists(cfg2.datasources, "appdb")#</cfoutput>

<!--- Datasource connectivity test --->
<cfset __configSet({datasources: {testdb: {backend: "sqlite"}}})>
<cfset okTest = __datasourceTest("testdb")>
<cfset missingTest = __datasourceTest("ghost")>
<cfoutput>dsTestOk=#okTest.verified#
dsTestMissing=#missingTest.error#</cfoutput>

<!--- Server runtime stats (+ admin-request filter argument) --->
<cfset si = __serverInfo()>
<cfset siFiltered = __serverInfo(true)>
<cfoutput>serverState=#si.state#|#si.version#|#si.requestsServed#|filteredLen=#ArrayLen(siFiltered.recentRequests)#</cfoutput>

<!--- Config reset restores defaults --->
<cfset __configSet({settings: {defaultOutputCharset: "ISO-8859-1"}})>reset=no
<cfset r2 = __configReset()>
<cfoutput>configResetCharset=#r2.settings.defaultOutputCharset#|#r2.settings.debugEnabled#</cfoutput>

<!--- Cache extension functions --->
<cfset CachePut("c1", "v")>
<cfoutput>cacheInfo=#__cacheInfo().totalEntries#|evict=#__cacheEvict("OBJECT", "c1").ok#|afterEvict=#__cacheInfo().totalEntries#|clear=#__cacheClear().ok#|compiledTemplates=#__cacheInfo().compiledTemplates#|compiledComponents=#__cacheInfo().compiledComponents#</cfoutput>

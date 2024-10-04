<!--- Byte-verified against CF 2025 (2026-08-09) once the "caching" package was
installed on the RDS host. Single-line file: the whitespace-only regions between
constructs are unambiguous, and the whole-page (self-closing) cache miss/hit
needs two requests so it is covered by the CfcacheTagTest unit tests. --->
<cfcache action="put" id="o1" value="V1"><cfcache action="get" id="o1" name="r"><cfoutput>get=#r#</cfoutput>|<cfcache action="put" id="m1" value="mv"><cfcache action="get" id="m1" name="rv" metadata="md"><cfoutput>md=#md.NAME#:#md.HITCOUNT#</cfoutput>|<cfcache action="flush" id="o1"><cfcache action="get" id="o1" name="r2"><cfoutput>miss=#isDefined("r2")#</cfoutput>|frag=<cfcache id="fa">A</cfcache><cfcache id="fb">B</cfcache>|strip=<cfcache id="sc" stripwhitespace="true">X		Y</cfcache>|dep=<cfset dep="v1"><cfcache dependson="dep">ONE</cfcache>

<!--- Temp-variant cleanup under loops (was BUGS.md "Temp-variant accumulation
     and a layout-sensitive heap corruption"): expression temporaries created
     in loops and try/catch bodies are freed per iteration / per try scope
     instead of living until the function returns. Output is deterministic;
     the memory/stack behavior is what changed. --->
<cfset t = 0>
<cfloop index="i" from="1" to="5000">
  <cfset s = {a:"x", b:[1,2,3]}>
  <cfset t = t + s.b[1]>
</cfloop>
<cfloop index="i" from="1" to="5000">
  <cftry>
    <cfset q = QueryNew("col")>
    <cfset QueryAddRow(q)>
    <cfthrow message="b">
    <cfcatch type="any"></cfcatch>
  </cftry>
  <cfset t = t + 1>
</cfloop>
<cfscript>
for (i = 1; i <= 5000; i++) {
    try {
        x = {p:1, q:[2,3]};
        throw "b2";
    } catch (any e) {
        t = t + 1;
    }
}
</cfscript>
<cfoutput>#t#</cfoutput>

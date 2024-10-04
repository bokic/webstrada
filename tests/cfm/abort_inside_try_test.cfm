<!--- abort inside cftry stays uncatchable (was BUGS.md #27) --->
T1:
<cfoutput>BEFORE|</cfoutput>
<cftry>
  <cfabort>
  <cfcatch type="any"><cfoutput>CAUGHT</cfoutput></cfcatch>
</cftry>
<cfoutput>AFTER</cfoutput>
|T2:
<cfoutput>B|</cfoutput>
<cftry>
  <cfabort>
  <cffinally><cfoutput>FIN</cfoutput></cffinally>
</cftry>
<cfoutput>A</cfoutput>
|T3:
NEST:
<cftry>
  <cftry>
    <cfabort>
    <cfcatch type="any"><cfoutput>INNER</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>OUTER</cfoutput></cfcatch>
</cftry>
<cfoutput>AFTER</cfoutput>
|T4:
S|<cfscript>writeOutput("X|"); try { abort; } catch (any e) { writeOutput("CAUGHT"); }</cfscript>
<cfoutput>|AFTER</cfoutput>

<!--- A <cftry> with neither <cfcatch> nor <cffinally> is rejected at compile
     time by CF (verified: the page 500s with "Context validation error in
     CFTRY block..."). This test is covered by the CLI unit tests; here we
     verify the valid cffinally-only and catch forms still compile. --->
<cftry><cfset q = 5><cffinally><cfoutput>FIN</cfoutput></cffinally></cftry>
|CATCH:
<cftry><cfthrow message="boom"><cfcatch type="any"><cfoutput>CAUGHT</cfoutput></cfcatch></cftry>

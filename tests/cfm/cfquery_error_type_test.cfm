<!--- cfquery database errors report type Database (was BUGS.md #30) --->
1:
<cftry>
  <cfquery name="q" datasource="webstrada">SELECT * FROM no_such_table</cfquery>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#]#cfcatch.message#</cfoutput></cfcatch>
</cftry>
|2:
<cftry>
  <cfquery name="q" datasource="webstrada">SELECT * FROM no_such_table</cfquery>
  <cfcatch type="database"><cfoutput>[DB]</cfoutput></cfcatch>
  <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
</cftry>
|3:
<cftry>
  <cfquery name="q" datasource="webstrada">SELECT * FROM no_such_table</cfquery>
  <cfcatch type="expression"><cfoutput>[EXPR]</cfoutput></cfcatch>
  <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
</cftry>
|4:
<cftry>
  <cfquery name="q" datasource="webstrada">SELECT FROM WHERE</cfquery>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#]#cfcatch.message#</cfoutput></cfcatch>
</cftry>

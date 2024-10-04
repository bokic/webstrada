<cfset sep = "|" />
<cftry>
  <cfset ImageNew("nonexistent_file_probe_xyz.png", 10, 10) /><cfoutput>NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>
<cfoutput>#sep#</cfoutput>
<cftry>
  <cfset ImageNew("", 0, 10) /><cfoutput>NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>
<cfoutput>#sep#</cfoutput>
<cftry>
  <cfset ImageNew("", 10, 0) /><cfoutput>NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>
<cfoutput>#sep#</cfoutput>
<cftry>
  <cfset ImageNew("", -5, 10, "rgb") /><cfoutput>NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>
<cfoutput>#sep#</cfoutput>
<cftry>
  <cfset ImageNew("", 10, 10, "rgba") /><cfoutput>NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>

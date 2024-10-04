<cfset sep = "|" />
<cfset img = ImageNew("", 4, 4, "rgb") />
<cftry>
  <cfset ImageWrite(img, "img_err_noext") /><cfoutput>NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>
<cfoutput>#sep#</cfoutput>
<cftry>
  <cfset ImageWrite(img, "img_err_q.jpg", 1.5, true) /><cfoutput>NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>
<cfoutput>#sep#</cfoutput>
<cftry>
  <cfset ImageWrite(img, "img_err_q2.jpg", -0.5, true) /><cfoutput>NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>

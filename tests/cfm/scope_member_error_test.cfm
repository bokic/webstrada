<cfapplication name="t" sessionmanagement="true">
<cftry><cfoutput>#session.x#</cfoutput><cfcatch type="any"><cfoutput>S1:[#cfcatch.message#]|</cfoutput></cfcatch></cftry>
<cftry><cfoutput>#application.y#</cfoutput><cfcatch type="any"><cfoutput>S2:[#cfcatch.message#]|</cfoutput></cfcatch></cftry>
<cftry><cfoutput>#variables.undefinedVar#</cfoutput><cfcatch type="any"><cfoutput>S3:[#cfcatch.message#]|</cfoutput></cfcatch></cftry>
<cftry><cfoutput>#url.undefinedVar#</cfoutput><cfcatch type="any"><cfoutput>S4:[#cfcatch.message#]|</cfoutput></cfcatch></cftry>
<cftry><cfoutput>#form.nope#</cfoutput><cfcatch type="any"><cfoutput>S5:[#cfcatch.message#]|</cfoutput></cfcatch></cftry>
<cftry><cfoutput>#server.nope#</cfoutput><cfcatch type="any"><cfoutput>S6:[#cfcatch.message#]|</cfoutput></cfcatch></cftry>
<cfset session.sa = structNew()>
<cftry><cfoutput>#session.sa.missing#</cfoutput><cfcatch type="any"><cfoutput>S7:[#cfcatch.message#]|</cfoutput></cfcatch></cftry>

<!--- Unqualified (unprefixed) name lookup must NOT search session/application
scopes either (they are never searched for unqualified names regardless of
`searchimplicitscopes`, per ColdFusion's searchScopes order). Verified against
CF 2025. --->
<cfapplication sessionmanagement="true">
<cfset session.ONLYSESS = "sessonly">
<cfset application.ONLYAPP = "apponly">
<cftry>
  <cfoutput>#ONLYSESS#</cfoutput>
<cfcatch type="any"><cfoutput>ERR_ONLYSESS</cfoutput></cfcatch>
</cftry>|
<cftry>
  <cfoutput>#ONLYAPP#</cfoutput>
<cfcatch type="any"><cfoutput>ERR_ONLYAPP</cfoutput></cfcatch>
</cftry>|
<cfoutput>#session.ONLYSESS#|#application.ONLYAPP#</cfoutput>

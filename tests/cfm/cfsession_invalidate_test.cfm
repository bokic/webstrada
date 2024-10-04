<cfapplication name="inv_test" sessionmanagement="true">
<cfset session.y = 2>
<cfset SessionInvalidate()>
<cfoutput>INV=#StructKeyExists(session, "y")#</cfoutput>

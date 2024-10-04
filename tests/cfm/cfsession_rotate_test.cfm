<cfapplication name="rot_test" sessionmanagement="true">
<cfset session.x = "keepme">
<cfset SessionRotate()>
<cfoutput>ROT=#session.x#</cfoutput>

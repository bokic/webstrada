<cfapplication name="stop_test" sessionmanagement="true">
<cfset application.x = 1>
<cfset session.y = 2>
<cfset ApplicationStop()>
<cfoutput>STOP=#StructKeyExists(application, "x")#:#StructKeyExists(session, "y")#</cfoutput>

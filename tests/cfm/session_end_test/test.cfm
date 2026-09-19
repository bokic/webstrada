<cfset application.endCount = 0>
<cfset application.lastEndedSessionVar = "">
<cfset session.myVar = "session_val_123">
<cfoutput>before:#application.endCount#,#application.lastEndedSessionVar#|</cfoutput>
<cfset sessionInvalidate()>
<cfoutput>after:#application.endCount#,#application.lastEndedSessionVar#</cfoutput>

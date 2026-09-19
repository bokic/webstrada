<cfset application.done = false>
<cfset session.foo = "bar">
<cfoutput>BEFORE|</cfoutput>
<cftry>
    <cfset sessionInvalidate()>
    <cfoutput>INVALIDATE_SUCCESS|</cfoutput>
    <cfcatch type="any">
        <cfoutput>INVALIDATE_THREW:#cfcatch.message#|</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>DONE:#application.done#</cfoutput>

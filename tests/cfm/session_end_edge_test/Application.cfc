<cfcomponent>
    <cfset this.name = "TestSessionEndEdgeApp">
    <cfset this.sessionManagement = true>

    <cffunction name="onApplicationStart">
        <cfset application.errCaught = false>
        <cfset application.done = false>
    </cffunction>

    <cffunction name="onSessionEnd">
        <cfargument name="SessionScope" required="true">
        <cfargument name="ApplicationScope" required="false">
        <!--- Raw output that should be discarded --->
        LEAKED_TEXT_SHOULD_BE_DISCARDED
        <cfset writeOutput("WRITEOUTPUT_SHOULD_BE_DISCARDED")>
        <cfset arguments.ApplicationScope.done = true>
        <!--- Throw an exception at the end to test exception suppression --->
        <cfthrow message="intentional error in onSessionEnd">
    </cffunction>
</cfcomponent>

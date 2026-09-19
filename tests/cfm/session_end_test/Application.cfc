<cfcomponent>
    <cfset this.name = "TestSessionEndApp">
    <cfset this.sessionManagement = true>

    <cffunction name="onApplicationStart">
        <cfset application.endCount = 0>
        <cfset application.lastEndedSessionVar = "">
    </cffunction>

    <cffunction name="onSessionEnd">
        <cfargument name="SessionScope" required="true">
        <cfargument name="ApplicationScope" required="false">
        <cfset arguments.ApplicationScope.endCount = arguments.ApplicationScope.endCount + 1>
        <cfif structKeyExists(arguments.SessionScope, "myVar")>
            <cfset arguments.ApplicationScope.lastEndedSessionVar = arguments.SessionScope.myVar>
        </cfif>
    </cffunction>
</cfcomponent>

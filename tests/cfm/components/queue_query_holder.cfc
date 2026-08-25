<cfcomponent>
    <cffunction name="read" access="public" returntype="numeric">
        <cfargument name="q" required="true">
        <cfloop query="arguments.q">
            <cfset var queue = createObject("component", "components.utilities.Queue")>
            <cfreturn arrayLen(queue.getElements())>
        </cfloop>
        <cfreturn -1>
    </cffunction>
</cfcomponent>

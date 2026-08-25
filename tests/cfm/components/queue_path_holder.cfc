<cfcomponent>
    <cfset variables.queues = structNew() />
    <cffunction name="add" access="public" output="false">
        <cfset variables.queues.event = createObject("component", "utilities.Queue") />
        <cfset variables.queues.event.addElement(structNew(), 5) />
    </cffunction>
    <cffunction name="read" access="public" output="false" returntype="numeric">
        <cfset var all = variables.queues.event.getElements() />
        <cfreturn arrayLen(all) />
    </cffunction>
</cfcomponent>

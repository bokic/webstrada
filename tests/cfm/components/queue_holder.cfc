<cfcomponent>
    <cfset variables.queues = structNew() />
    <cffunction name="add" access="public" output="false">
        <cfargument name="name" required="true" />
        <cfif NOT structKeyExists(variables.queues, arguments.name)>
            <cfset variables.queues[arguments.name] = createObject("component", "queue_exact") />
        </cfif>
        <cfset variables.queues[arguments.name].addElement(structNew(), 5) />
    </cffunction>
    <cffunction name="read" access="public" output="false" returntype="numeric">
        <cfargument name="name" required="true" />
        <cfset var all = variables.queues[arguments.name].getElements() />
        <cfreturn arrayLen(all) />
    </cffunction>
</cfcomponent>

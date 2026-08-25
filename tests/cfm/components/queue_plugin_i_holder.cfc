<cfcomponent>
    <cfset variables.queue = createObject("component", "components.utilities.Queue")>
    <cffunction name="read" access="public" returntype="numeric">
        <cfset var i = "">
        <cfset variables.queue.addElement(structNew(), 5)>
        <cfset var all = variables.queue.getElements()>
        <cfreturn arrayLen(all)>
    </cffunction>
</cfcomponent>

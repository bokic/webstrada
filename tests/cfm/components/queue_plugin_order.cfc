<cfcomponent>
	<cfset variables.queue = createObject("component", "utilities.Queue") />
	<cffunction name="add" access="public">
		<cfargument name="value" required="true" />
		<cfargument name="priority" type="numeric" required="true" />
		<cfset variables.queue.addElement(arguments.value, arguments.priority) />
	</cffunction>
	<cffunction name="read" access="public" returntype="array">
		<cfreturn variables.queue.getElements() />
	</cffunction>
</cfcomponent>

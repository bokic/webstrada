<cfcomponent>
	<cfset variables.queues = structNew() />
	<cffunction name="addListener" access="public">
		<cfargument name="eventName" type="string" required="true" />
		<cfset variables.queues[arguments.eventName] = createObject("component", "utilities.Queue") />
		<cfset variables.queues[arguments.eventName].addElement(structNew(), 5) />
	</cffunction>
	<cffunction name="read" access="public" returntype="array">
		<cfargument name="eventName" type="string" required="true" />
		<cfreturn variables.queues[arguments.eventName].getElements() />
	</cffunction>
</cfcomponent>

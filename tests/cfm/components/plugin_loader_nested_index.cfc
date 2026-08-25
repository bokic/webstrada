<cfcomponent>
	<cffunction name="run" access="public" returntype="string">
		<cfset var i = 0 />
		<cfset var out = "" />
		<cfset var events = [{name="SubscriptionHandler", priority=5}] />
		<cfloop index="i" list="SubscriptionHandler,Links">
			<cfset out = listAppend(out, add(events)) />
		</cfloop>
		<cfreturn out />
	</cffunction>
	<cffunction name="add" access="public" returntype="string">
		<cfargument name="events" type="array" required="true" />
		<cfset var i = 0 />
		<cfset var out = "" />
		<cfloop index="i" from="1" to="#arrayLen(arguments.events)#">
			<cfset out = listAppend(out, arguments.events[i].priority) />
		</cfloop>
		<cfreturn out />
	</cffunction>
</cfcomponent>

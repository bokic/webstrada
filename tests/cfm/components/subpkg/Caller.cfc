<cfcomponent>
	<cffunction name="callSub" access="public" returntype="string">
		<cfset var h = createObject("component", "Helper") />
		<cfreturn h.helper() />
	</cffunction>
</cfcomponent>

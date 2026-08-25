<cfcomponent>
	<cffunction name="run" access="public" returntype="string">
		<cfset var i = 0 />
		<cfset var out = "" />
		<cfloop index="i" from="1" to="2">
			<cfset out = listAppend(out, i) />
		</cfloop>
		<cfloop index="i" list="SubscriptionHandler,Links,Statistics">
			<cfset out = listAppend(out, i) />
		</cfloop>
		<cfreturn out />
	</cffunction>
</cfcomponent>

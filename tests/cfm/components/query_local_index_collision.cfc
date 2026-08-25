<cfcomponent>
	<cffunction name="run" access="public" returntype="string">
		<cfset var i = 0 />
		<cfset var values = ["ok"] />
		<cfset var q = queryNew("i", "integer", [[0]]) />
		<cfset var out = "" />
		<cfloop query="q">
			<cfloop from="1" to="1" index="i">
				<cfset out = values[i] />
			</cfloop>
		</cfloop>
		<cfreturn out />
	</cffunction>
</cfcomponent>

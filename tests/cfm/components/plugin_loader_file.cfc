<cfcomponent>
	<cffunction name="run" access="public" returntype="string">
		<cfargument name="list" type="string" required="true" />
		<cfargument name="path" type="string" required="true" />
		<cfset var i = 0 />
		<cfset var xml = "" />
		<cfset var out = "" />
		<cfloop index="i" list="#arguments.list#">
			<cftry>
				<cffile action="read" file="#arguments.path##i#/plugin.xml" variable="xml" />
				<cfset out = listAppend(out, i) />
				<cfcatch type="any"><cfset out = listAppend(out, cfcatch.message) /></cfcatch>
			</cftry>
		</cfloop>
		<cfreturn out />
	</cffunction>
</cfcomponent>

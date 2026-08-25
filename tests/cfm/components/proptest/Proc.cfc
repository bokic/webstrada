<cfcomponent>
	<cffunction name="processEvent" output="false" returntype="any">
		<cfargument name="event" type="any" required="true" />
		<cfset var data = "" />
		<cfset var eventName = arguments.event.name />
		<cfif eventName EQ "beforeHtmlHeadEnd">
			<cfset data = arguments.event.outputData />
			<cfset data = data & '<link rel="stylesheet" href="/x.css" />' />
			<cfset arguments.event.outputData = data />
		</cfif>
		<cfreturn arguments.event />
	</cffunction>
</cfcomponent>

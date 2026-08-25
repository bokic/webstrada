<cfcomponent name="Event">
	<cfset this.name = "" />
	<cfset this.outputdata = "" />
	<cffunction name="getOutputData" access="public" output="false" returntype="any">
		<cfreturn this.outputdata />
	</cffunction>
	<cffunction name="setOutputData" access="public" output="false" returntype="void">
		<cfargument name="data" type="any" required="true" />
		<cfset this.outputdata = arguments.data />
	</cffunction>
</cfcomponent>

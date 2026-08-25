<cfcomponent>
    <cffunction name="check" returntype="struct" output="false">
        <cfargument name="key" type="string" required="true">
        <cfset var result = structnew()>
        <cfset result.value = arguments.key>
        <cfreturn result>
    </cffunction>
</cfcomponent>

<cfcomponent>
    <cffunction name="read" access="public" returntype="string" output="false">
        <cfargument name="value" type="string" required="true" />
        <cfreturn arguments.value />
    </cffunction>
</cfcomponent>

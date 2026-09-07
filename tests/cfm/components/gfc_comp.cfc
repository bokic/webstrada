<cfcomponent>
    <cfset variables.inBody = GetFunctionCalledName()>
    <cffunction name="getMyName" access="public" returntype="string">
        <cfreturn GetFunctionCalledName()>
    </cffunction>
    <cffunction name="getBodyName" access="public" returntype="string">
        <cfreturn variables.inBody>
    </cffunction>
</cfcomponent>

<cfcomponent>
    <cfset this.currentRole = createObject("component", "NestedChild").init()>
    <cffunction name="init" access="public" output="false">
        <cfreturn this>
    </cffunction>
    <cffunction name="getCurrentUser" access="public" output="false">
        <cfreturn this>
    </cffunction>
</cfcomponent>

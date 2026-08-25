<cfcomponent>
    <cfset this.preferences = createObject("component", "NestedPreferences").init()>
    <cffunction name="init" access="public" output="false">
        <cfreturn this>
    </cffunction>
</cfcomponent>

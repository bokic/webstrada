<cfcomponent>
    <cffunction name="innerThrow" access="public" returntype="void">
        <cfthrow type="compErr" message="boom">
    </cffunction>
    <cffunction name="outerCall" access="public" returntype="void">
        <cfset innerThrow()>
    </cffunction>
    <cffunction name="scriptThrow" access="public" returntype="void">
        <cfthrow type="compErr2" message="boom2">
    </cffunction>
</cfcomponent>

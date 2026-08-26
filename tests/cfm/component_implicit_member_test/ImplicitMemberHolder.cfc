<cfcomponent name="ImplicitMemberHolder">
    <cfscript>
        this.mappings["/org/mangoblog"] = "components";
    </cfscript>
    <cffunction name="value" returntype="string" output="false">
        <cfreturn this.mappings["/org/mangoblog"]>
    </cffunction>
</cfcomponent>

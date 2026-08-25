<cfcomponent>
    <cfset this.toRemove = 1>
    <cffunction name="remove" returntype="string">
        <cfreturn StructDelete(this, "toRemove", true) & "|" & StructKeyExists(this, "toRemove")>
    </cffunction>
</cfcomponent>

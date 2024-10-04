<cfcomponent extends="grandparent">
    <cffunction name="whoami">
        <cfreturn "Parent -> " & super.whoami()>
    </cffunction>
</cfcomponent>

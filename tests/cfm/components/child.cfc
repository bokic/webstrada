<cfcomponent extends="parent">
    <cffunction name="whoami">
        <cfreturn "Child -> " & super.whoami()>
    </cffunction>
</cfcomponent>

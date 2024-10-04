<cfcomponent extends="super_parent">
    <cffunction name="greet">
        <cfreturn "Child: " & super.greet()>
    </cffunction>
    <cffunction name="getValue">
        <cfreturn super.getValue() + 10>
    </cffunction>
</cfcomponent>

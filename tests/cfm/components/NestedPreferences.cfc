<cfcomponent>
    <cffunction name="init" access="public" output="false">
        <cfreturn this>
    </cffunction>
    <cffunction name="get" access="public" output="false">
        <cfargument name="path" required="true">
        <cfargument name="key" required="true">
        <cfargument name="default" required="false" default="">
        <cfreturn arguments.path & ":" & arguments.key & ":" & arguments.default>
    </cffunction>
</cfcomponent>

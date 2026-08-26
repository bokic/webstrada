<cfcomponent name="TagComponent">
    <cffunction name="reveal" output="false" returntype="string">
        <cfreturn secret()>
    </cffunction>
    <cfscript>
        private function secret() { return "PRIVATE-OK"; }
    </cfscript>
</cfcomponent>

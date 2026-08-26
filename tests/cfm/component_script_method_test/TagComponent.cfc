<cfcomponent name="TagComponent">
    <cffunction name="init" output="false" returntype="string">
        <cfreturn structBlend()>
    </cffunction>
    <cfscript>
        function structBlend() { return "OK"; }
    </cfscript>
</cfcomponent>

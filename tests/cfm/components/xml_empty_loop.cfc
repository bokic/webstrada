<cfcomponent>
    <cffunction name="countPageTemplates" access="public" output="false" returntype="numeric">
        <cfargument name="xmlText" type="string" required="true" />
        <cfset var data = xmlParse(arguments.xmlText) />
        <cfset var count = 0 />
        <cfset var i = 0 />
        <cfloop from="1" to="#arrayLen(data.skin.pageTemplates.xmlChildren)#" index="i">
            <cfset count = count + 1 />
        </cfloop>
        <cfreturn count />
    </cffunction>
</cfcomponent>

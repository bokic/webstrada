<cfcomponent>
    <cfset variables.elements = arrayNew(1) />

    <cffunction name="getElements" output="false" returntype="array">
        <cfset var result = arrayNew(1) />
        <cfset var i = 0 />
        <cfloop index="i" from="1" to="#arrayLen(variables.elements)#">
            <cfset result[i] = variables.elements[i].obj />
        </cfloop>
        <cfreturn result />
    </cffunction>
</cfcomponent>

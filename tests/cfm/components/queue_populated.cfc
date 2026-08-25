<cfcomponent>
    <cfset variables.elements = arrayNew(1) />
    <cffunction name="addElement" access="public" output="false">
        <cfargument name="value" required="true" />
        <cfset var item = structNew() />
        <cfset item.obj = arguments.value />
        <cfset arrayAppend(variables.elements, item) />
    </cffunction>
    <cffunction name="getElements" access="public" output="false" returntype="array">
        <cfset var result = arrayNew(1) />
        <cfset var i = 0 />
        <cfloop index="i" from="1" to="#arrayLen(variables.elements)#">
            <cfset result[i] = variables.elements[i].obj />
        </cfloop>
        <cfreturn result />
    </cffunction>
</cfcomponent>

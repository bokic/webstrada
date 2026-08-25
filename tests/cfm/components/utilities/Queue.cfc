<cfcomponent name="Queue">
    <cfset variables.elements = arrayNew(1) />
    <cffunction name="getElements" access="public" output="false" returntype="any">
        <cfset var tempArray = arrayNew(1) />
        <cfset var i = 0 />
        <cfloop index="i" from="1" to="#arrayLen(variables.elements)#">
            <cfset tempArray[i] = variables.elements[i].obj />
        </cfloop>
        <cfreturn tempArray />
    </cffunction>
    <cffunction name="addElement" access="public" output="false" returntype="any">
        <cfargument name="element" required="false" />
        <cfargument name="priority" required="false" />
        <cfset var elementObj = structNew() />
        <cfset elementObj.obj = arguments.element />
        <cfset elementObj.priority = arguments.priority />
        <cfset arrayAppend(variables.elements, elementObj) />
    </cffunction>
</cfcomponent>

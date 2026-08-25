<cfcomponent name="Queue">
    <cfproperty name="elements" type="any" />
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
        <cfargument name="element" type="any" required="false" />
        <cfargument name="priority" type="numeric" required="false" />
        <cfset var i = 0 />
        <cfset var elementObj = structNew() />
        <cfset var lastIndex = arrayLen(variables.elements) />
        <cfset elementObj.obj = arguments.element />
        <cfset elementObj.priority = arguments.priority />
        <cfif lastIndex EQ 0 OR variables.elements[lastIndex].priority GTE arguments.priority>
            <cfset arrayAppend(variables.elements, elementObj) />
        <cfelse>
            <cfloop index="i" from="1" to="#lastIndex#">
                <cfif variables.elements[i].priority LT arguments.priority>
                    <cfset arrayInsertAt(variables.elements, i, elementObj) />
                    <cfbreak />
                </cfif>
            </cfloop>
        </cfif>
        <cfreturn />
    </cffunction>
</cfcomponent>

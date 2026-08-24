<cffunction name="acceptStruct" returntype="struct">
    <cfargument name="s" type="struct" required="true">
    <cfreturn s>
</cffunction>

<cfset p = CreateObject("component", "components/person")>
<cfset p.init("Alice")>
<cfset res = acceptStruct(p)>
<cfoutput>#res.getName()#|#isStruct(res)#|#isObject(res)#</cfoutput>

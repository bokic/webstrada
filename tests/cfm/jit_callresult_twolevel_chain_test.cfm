<cffunction name="fn" returntype="any">
	<cfreturn DeserializeJSON('{"a":{"b":"v"}}')>
</cffunction>
<cfset x1 = fn().a.b><cfoutput>#IsStruct(x1)#:#x1#</cfoutput>
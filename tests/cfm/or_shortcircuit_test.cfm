<cfset elements = arraynew(1) />
<cfset lastIndex = arraylen(elements) />
<cfif lastIndex EQ 0 OR elements[lastIndex].priority GTE 5>
	<cfset arrayappend(elements, "END") />
</cfif>
<cfoutput>#arraylen(elements)#</cfoutput>

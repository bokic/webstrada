<!--- CallStackGet: component method frame carries the uppercased method name (the
     component file keeps its name on the verify host, so its template is
     byte-comparable; the page frame's template is a renamed temp file) --->
<cfset c = CreateObject("component", "components/callstack_comp")>
<cfset cs = c.getStack()>
<cfoutput>#arrayLen(cs)#</cfoutput>
<cfloop index="i" from="1" to="#arrayLen(cs)#">
<cfoutput>[#i#]#cs[i].function#:#cs[i].lineNumber#</cfoutput>
<cfif i eq 1><cfoutput>:#GetFileFromPath(cs[i].template)#</cfoutput></cfif>
</cfloop>

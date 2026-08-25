<cfset q = queryNew("i", "integer", [[0]])>
<cfloop query="q">
    <cfset queue = createObject("component", "components.utilities.Queue")>
    <cfoutput>#arrayLen(queue.getElements())#</cfoutput>
</cfloop>

<cfset queue = new components.queue_exact() />
<cfset queue.addElement(structNew(), 5) />
<cfset result = queue.getElements() />
<cfoutput>#arrayLen(result)#</cfoutput>

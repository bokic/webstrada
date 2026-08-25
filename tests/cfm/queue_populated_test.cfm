<cfset queue = new components.queue_populated() />
<cfset queue.addElement(structNew()) />
<cfset result = queue.getElements() />
<cfoutput>#arrayLen(result)#</cfoutput>

<cfset queue = new components.queue_empty() />
<cfset first = queue.getElements() />
<cfset second = queue.getElements() />
<cfoutput>#arrayLen(first)#:#arrayLen(second)#</cfoutput>

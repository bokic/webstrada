<cfset holder = structNew() />
<cfset holder.queue = new components.queue_empty() />
<cfset result = holder.queue.getElements() />
<cfoutput>#arrayLen(result)#</cfoutput>

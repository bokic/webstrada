<cfset queue = new components.queue_empty() />
<cfoutput>empty=#arrayLen(queue.getElements())#</cfoutput>

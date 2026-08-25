<cfset holder = new components.queue_holder() />
<cfset holder.add("event") />
<cfoutput>#holder.read("event")#</cfoutput>

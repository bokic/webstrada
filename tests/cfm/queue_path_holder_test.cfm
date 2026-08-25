<cfset holder = new components.queue_path_holder() />
<cfset holder.add() />
<cfoutput>#holder.read()#</cfoutput>

<cfset q = queryNew("i", "integer", [[0]])>
<cfset holder = new components.queue_query_holder()>
<cfoutput>#holder.read(q)#</cfoutput>

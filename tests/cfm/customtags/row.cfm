<cfif thisTag.executionMode eq "start">
[ROW:<cfoutput>#attributes.rname#</cfoutput>]
<cfassociate basetag="cf_collector" datacollection="assoc">
<cfelse>
[ROW-END]
</cfif>

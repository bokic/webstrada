<cfif thisTag.executionMode eq "start">
[S2:<cfoutput>#attributes.nm#</cfoutput>]
<cfelse>
[E2:<cfoutput>#thisTag.generatedContent#</cfoutput>]
</cfif>

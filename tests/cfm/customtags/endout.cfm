<cfif thisTag.executionMode eq "start">
[S:<cfoutput>#attributes.nm#</cfoutput>]
<cfelse>
[E-BEFORE:<cfoutput>#thisTag.generatedContent#</cfoutput>]
<cfset thisTag.generatedContent = ucase(thisTag.generatedContent)>
[E-AFTER]
</cfif>

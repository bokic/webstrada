<cfif thisTag.executionMode eq "start">
[WRAPPER_START:<cfoutput>#attributes.title#</cfoutput>]
<cfelse>
[WRAPPER_END:<cfoutput>#thisTag.generatedContent#</cfoutput>]
<cfset thisTag.generatedContent = ucase(thisTag.generatedContent)>
</cfif>

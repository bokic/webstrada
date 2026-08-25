<cfoutput>[MOD:start:#thisTag.executionMode#:#attributes.nm#]</cfoutput>
<cfif thisTag.executionMode eq "end">
[MOD-END:<cfoutput>#thisTag.generatedContent#</cfoutput>]
<cfset thisTag.generatedContent = ucase(thisTag.generatedContent)>
</cfif>

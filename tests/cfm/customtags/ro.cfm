<cfif thisTag.executionMode eq "start">
<cftry>
<cfset thisTag.hasEndTag = "NO">
<cfcatch any><cfoutput>[caught:#cfcatch.type#:#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
<cfelse>
<cftry>
<cfset thisTag.executionMode = "foo">
<cfcatch any><cfoutput>[e:#cfcatch.type#:#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
</cfif>

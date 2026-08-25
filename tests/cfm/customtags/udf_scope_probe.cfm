<cfif thisTag.executionMode EQ "start">
    <cfset result = createObject("component", "components.CustomTagUdfScopeProbe").check("ok")>
    <cfoutput>#result.value#</cfoutput>
</cfif>

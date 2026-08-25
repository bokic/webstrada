<cfset probe = createObject("component", "components.IncludeArgumentProbe") />
<cfoutput>#probe.read("from-include")#</cfoutput>

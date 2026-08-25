<cfoutput>[OUTER:#thisTag.executionMode#]</cfoutput>
<cfif thisTag.executionMode eq "start">
<cfmodule template="inner.cfm">INNERBODY</cfmodule>
</cfif>

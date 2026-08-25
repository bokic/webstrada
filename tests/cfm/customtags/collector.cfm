<cfif thisTag.executionMode eq "start">
[COLLECTOR-START]
<cfmodule template="row.cfm" rname="R1">body1</cfmodule>
[AFTER-ROW]
<cfoutput>[COLLECTED:#serializeJSON(thisTag.assoc)#]</cfoutput>
<cfelse>
[COLLECTOR-END]
</cfif>

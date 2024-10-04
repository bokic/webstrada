<cftry>
<cfscript>
arr = [3, 1, 2];
r1 = arr.push(5);
writeOutput("push=" & r1 & "|len=" & ArrayLen(arr) & "|list=" & arr.toList());
</cfscript>
<cfcatch type="any"><cfoutput>ERR:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

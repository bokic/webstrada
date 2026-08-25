<cfif thisTag.executionMode eq "start">
[START-OUT]
<cfelse>
[END-BEFORE]<cfexit>[END-AFTER]
</cfif>

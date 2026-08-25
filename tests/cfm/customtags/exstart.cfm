<cfif thisTag.executionMode eq "start">
[EX-START]<cfexit>[EX-START-AFTER]
<cfelse>
[END-SHOULD-NOT-RUN]
</cfif>

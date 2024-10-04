<cfscript>
writeOutput("P1=" & IsObject(pi) & ",");
writeOutput("P2=" & IsSimpleValue(pi) & ",");
writeOutput("P3=" & Left(ToString(pi), 32) & ",");
writeOutput("P4=" & IsObject((abs)) & ",");
writeOutput("P5=" & IsObject(len) & ",");
</cfscript>
<cfset pi = 99><cfscript>writeOutput("P6=" & pi & ",");</cfscript><cfscript>writeOutput("P7=" & IsObject(pi) & ",");</cfscript>

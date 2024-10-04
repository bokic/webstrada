<cfscript>
writeOutput("1:[" & GetException("anything") & "]");
writeOutput("2:[" & GetException(5) & "]");
writeOutput("3:[" & GetException(structNew()) & "]");
writeOutput("4:[" & GetException([]) & "]");
</cfscript>

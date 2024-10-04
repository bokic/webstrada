<cfoutput>#[1,2].toList()#|#{a:1}.keyList()#|#[1,2,3].len()#</cfoutput>
<cfscript>
writeOutput("|" & [4,5].toList() & "|" & {b:2}.keyList());
</cfscript>

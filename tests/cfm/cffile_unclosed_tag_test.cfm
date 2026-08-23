<cfset base = "/tmp/webstrada_cffile_unclosed">
<cfif DirectoryExists(base)><cfdirectory action="delete" directory="#base#" recurse="true"></cfdirectory></cfif>
<cfdirectory action="create" directory="#base#">
<cffile action="write" file="#base#/t.txt" output="hello">
<cffile action="read" file="#base#/t.txt" variable="r1">
<cfoutput>READ:#r1#</cfoutput>
<cfoutput>|AFTER:#Len(r1)#</cfoutput>
<cfset afterTag = 42 />
<cfoutput>|VARAFTER:#afterTag#</cfoutput>
<cffile action="read" file="#base#/t.txt" variable="r2"></cffile>
<cfoutput>|CLOSED:#r2#</cfoutput>

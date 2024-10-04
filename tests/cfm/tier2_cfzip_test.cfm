<!--- cfzip byte-verification needs the CF `zip` package, installed on the RDS
     host 2026-08-09 (see BUGS_CF.md). The raw list order is archive-iteration
     dependent, so the test sorts the collected names; dateLastModified is
     creation-time dependent and is compared via DateFormat normalization. --->
<cfset base = "/tmp/webstrada_cfzip_test">
<cfset outdir = "/tmp/webstrada_cfzip_out">
<cfif DirectoryExists(base)><cfdirectory action="delete" directory="#base#" recurse="true"></cfdirectory></cfif>
<cfif DirectoryExists(outdir)><cfdirectory action="delete" directory="#outdir#" recurse="true"></cfdirectory></cfif>
<cfif FileExists("#base#.zip")><cffile action="delete" file="#base#.zip"></cffile></cfif>
<cfif FileExists("#base#_content.zip")><cffile action="delete" file="#base#_content.zip"></cffile></cfif>
<cfdirectory action="create" directory="#base#">
<cffile action="write" file="#base#/aa.txt" output="hello aa" addnewline="no"></cffile>
<cffile action="write" file="#base#/sub.bin" output="BINARYDATA" addnewline="no"></cffile>
<cfdirectory action="create" directory="#base#/subdir">
<cffile action="write" file="#base#/subdir/deep.txt" output="deep" addnewline="no"></cffile>

<!--- zip a directory source: entries are relative to the source directory --->
<cfzip action="zip" file="#base#.zip" source="#base#"></cfzip>

<!--- list: record count + columnList + sorted names/dirs --->
<cfzip action="list" file="#base#.zip" name="q"></cfzip>
<cfset names = "">
<cfset dirs = "">
<cfloop query="q"><cfset names = ListAppend(names, q.name)><cfset dirs = ListAppend(dirs, q.directory)></cfloop>
<cfoutput>list:#q.recordCount#|#q.columnList#|#ListSort(names,'text')#|#ListSort(dirs,'text')#</cfoutput>

<!--- list with showDirectory --->
<cfzip action="list" file="#base#.zip" name="q" showDirectory="yes"></cfzip>
<cfset names2 = "">
<cfloop query="q"><cfset names2 = ListAppend(names2, q.name)></cfloop>
<cfoutput>|showdir:#q.recordCount#:#ListSort(names2,'text')#</cfoutput>

<!--- read an entry as text --->
<cfzip action="read" file="#base#.zip" entrypath="aa.txt" variable="v1"></cfzip>
<cfoutput>|read:#Trim(v1)#</cfoutput>

<!--- read an entry as binary --->
<cfzip action="readBinary" file="#base#.zip" entrypath="sub.bin" variable="v2"></cfzip>
<cfoutput>|readbin:#IsBinary(v2)#:#Len(v2)#</cfoutput>

<!--- unzip to an existing directory --->
<cfdirectory action="create" directory="#outdir#"></cfdirectory>
<cfzip action="unzip" file="#base#.zip" destination="#outdir#"></cfzip>
<cfoutput>|unzip:#FileExists("#outdir#/aa.txt")#:#FileExists("#outdir#/subdir/deep.txt")#</cfoutput>

<!--- unzip to a non-existent directory throws --->
<cftry>
<cfzip action="unzip" file="#base#.zip" destination="#base#_missing"></cfzip>
<cfoutput>|unzipmissing:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|unzipmissing:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- zip with cfzipparam content --->
<cfset z2 = "#base#_content.zip">
<cfzip action="zip" file="#z2#">
  <cfzipparam content="hello content" entrypath="hello.txt">
</cfzip>
<cfzip action="read" file="#z2#" entrypath="hello.txt" variable="v3"></cfzip>
<cfoutput>|content:#Trim(v3)#</cfoutput>

<!--- zip with cfzipparam source + prefix --->
<cfzip action="zip" file="#z2#">
  <cfzipparam source="#base#/aa.txt" entrypath="pfx/aa.txt">
</cfzip>
<cfzip action="read" file="#z2#" entrypath="pfx/aa.txt" variable="v4"></cfzip>
<cfoutput>:#Trim(v4)#</cfoutput>

<!--- delete an entry --->
<cfzip action="delete" file="#base#.zip" entrypath="sub.bin"></cfzip>
<cfzip action="list" file="#base#.zip" name="q"></cfzip>
<cfset names3 = "">
<cfloop query="q"><cfset names3 = ListAppend(names3, q.name)></cfloop>
<cfoutput>|afterdelete:#q.recordCount#:#ListSort(names3,'text')#</cfoutput>

<!--- a missing entry throws --->
<cftry>
<cfzip action="read" file="#base#.zip" entrypath="nope/x.txt" variable="v5"></cfzip>
<cfoutput>|missentry:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|missentry:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- cleanup --->
<cffile action="delete" file="#base#.zip"></cffile>
<cffile action="delete" file="#z2#"></cffile>
<cfdirectory action="delete" directory="#base#" recurse="true"></cfdirectory>
<cfdirectory action="delete" directory="#outdir#" recurse="true"></cfdirectory>
<cfoutput>|final:#DirectoryExists(base)#:#DirectoryExists(outdir)#</cfoutput>

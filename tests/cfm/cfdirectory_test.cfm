<cfset base = "/tmp/webstrada_cfdir_test">
<cfif DirectoryExists(base)><cfdirectory action="delete" directory="#base#" recurse="true"></cfdirectory></cfif>
<cfset renamed = "/tmp/webstrada_cfdir_test_renamed">
<cfif DirectoryExists(renamed)><cfdirectory action="delete" directory="#renamed#" recurse="true"></cfdirectory></cfif>
<cfdirectory action="create" directory="#base#">
<cffile action="write" file="#base#/aa.txt" output="aaaa" addnewline="no"></cffile>
<cffile action="write" file="#base#/sub.bin" output="binarycontent" addnewline="no"></cffile>
<cfdirectory action="create" directory="#base#/subdir">
<cffile action="write" file="#base#/subdir/c.cfm" output="x" addnewline="no"></cffile>
<cffile action="write" file="#base#/subdir/d.bak" output="y" addnewline="no"></cffile>
<cffile action="write" file="#base#/e.txt" output="ee" addnewline="no"></cffile>

<!--- basic list: record count + columnList (raw order is filesystem-dependent,
     so the sorted run is compared for the order) --->
<cfdirectory directory="#base#" action="list" name="q">
<cfoutput>list:#q.recordCount#|#q.columnList#</cfoutput>
<cfdirectory directory="#base#" action="list" name="q" sort="name">
<cfoutput>|</cfoutput>
<cfloop query="q"><cfoutput>#q.name#:#q.type#:#q.mode#:#q.attributes#:#q.link#;</cfoutput></cfloop>

<!--- type filter --->
<cfdirectory directory="#base#" action="list" name="q" type="file" sort="name">
<cfoutput>|typefile:#q.recordCount#:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>
<cfdirectory directory="#base#" action="list" name="q" type="dir" sort="name">
<cfoutput>|typedir:#q.recordCount#:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>

<!--- filter --->
<cfdirectory directory="#base#" action="list" name="q" filter="*.txt" sort="name">
<cfoutput>|filter1:#q.recordCount#:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>
<cfdirectory directory="#base#" action="list" name="q" filter="*.txt|*.cfm" sort="name">
<cfoutput>|filter2:#q.recordCount#:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>
<cfdirectory directory="#base#" action="list" name="q" filter="*.*" sort="name">
<cfoutput>|filterstar:#q.recordCount#:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>

<!--- recurse: record count + a sorted order (raw iteration order is
     filesystem-dependent, so only the count and a sorted run are compared) --->
<cfdirectory directory="#base#" action="list" name="q" recurse="true">
<cfoutput>|recurse:#q.recordCount#</cfoutput>
<cfdirectory directory="#base#" action="list" name="q" recurse="true" sort="directory asc, name">
<cfoutput>|recurse_sorted:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#/#q.directory#;</cfoutput></cfloop>

<!--- sort --->
<cfdirectory directory="#base#" action="list" name="q" sort="name desc">
<cfoutput>|sortdesc:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>
<cfdirectory directory="#base#" action="list" name="q" sort="type asc, name desc">
<cfoutput>|sorttype:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>

<!--- file sizes (type=file only; directory sizes are filesystem-dependent) --->
<cfdirectory directory="#base#" action="list" name="q" type="file" sort="size asc, name">
<cfoutput>|sizes:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#:#q.size#;</cfoutput></cfloop>

<!--- listinfo=name (sorted; raw order is filesystem-dependent) --->
<cfdirectory directory="#base#" action="list" name="q" listinfo="name" sort="name">
<cfoutput>|listinfo:#q.columnList#:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>
<cfdirectory directory="#base#" action="list" name="q" listinfo="name" recurse="true" sort="name">
<cfoutput>|listinforec:</cfoutput>
<cfloop query="q"><cfoutput>#q.name#;</cfoutput></cfloop>

<!--- listing a non-existent directory yields an empty query --->
<cfdirectory directory="#base#/nonexistent" action="list" name="q">
<cfoutput>|missing:#q.recordCount#</cfoutput>

<!--- create existing directory throws --->
<cftry>
<cfdirectory action="create" directory="#base#">
<cfoutput>|createexist:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|createexist:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- delete a non-existent directory throws --->
<cftry>
<cfdirectory action="delete" directory="#base#/nope">
<cfoutput>|delmissing:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|delmissing:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- rename --->
<cfdirectory action="rename" directory="#base#" newdirectory="#renamed#">
<cfoutput>|renamed:#DirectoryExists(renamed)#:#DirectoryExists(base)#</cfoutput>

<!--- invalid type attribute throws --->
<cftry>
<cfdirectory directory="#renamed#" action="list" name="q" type="bogus">
<cfoutput>|badtype:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|badtype:#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch>
</cftry>

<!--- cleanup --->
<cfdirectory action="delete" directory="#renamed#" recurse="true">
<cfoutput>|final:#DirectoryExists(renamed)#</cfoutput>

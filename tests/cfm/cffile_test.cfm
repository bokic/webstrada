<cfset base = "/tmp/webstrada_cffile_test">
<cfif DirectoryExists(base)><cfdirectory action="delete" directory="#base#" recurse="true"></cfdirectory></cfif>
<cfdirectory action="create" directory="#base#">

<!--- write with output + default addnewline (appends newline) --->
<cffile action="write" file="#base#/w1.txt" output="aaaa"></cffile>
<cffile action="read" file="#base#/w1.txt" variable="c1"></cffile>
<cfoutput>write:#Len(c1)#:#Trim(c1)#</cfoutput>

<!--- write with addnewline=no --->
<cffile action="write" file="#base#/w2.txt" output="bbbb" addnewline="no"></cffile>
<cffile action="read" file="#base#/w2.txt" variable="c2"></cffile>
<cfoutput>|writenonl:#Len(c2)#:#c2#</cfoutput>

<!--- write with a tag body --->
<cffile action="write" file="#base#/w3.txt" addnewline="no">BODY-CONTENT</cffile>
<cffile action="read" file="#base#/w3.txt" variable="c3"></cffile>
<cfoutput>|writebody:#c3#</cfoutput>

<!--- append to an existing file (adds newline before the append) --->
<cffile action="append" file="#base#/w1.txt" output="XXXX"></cffile>
<cffile action="read" file="#base#/w1.txt" variable="c4"></cffile>
<cfoutput>|append:#Len(c4)#</cfoutput>

<!--- append to a non-existent file creates it --->
<cffile action="append" file="#base#/newfile.txt" output="ZZ" addnewline="no"></cffile>
<cffile action="read" file="#base#/newfile.txt" variable="c5"></cffile>
<cfoutput>|appendnew:#c5#</cfoutput>

<!--- readBinary --->
<cffile action="readBinary" file="#base#/w2.txt" variable="bin"></cffile>
<cfoutput>|readbin:#IsBinary(bin)#:#Len(bin)#</cfoutput>

<!--- copy --->
<cffile action="copy" source="#base#/w2.txt" destination="#base#/copy.txt"></cffile>
<cffile action="read" file="#base#/copy.txt" variable="c6"></cffile>
<cfoutput>|copy:#FileExists("#base#/copy.txt")#:#c6#</cfoutput>

<!--- rename --->
<cffile action="rename" source="#base#/copy.txt" destination="#base#/renamed.txt"></cffile>
<cfoutput>|rename:#FileExists("#base#/renamed.txt")#:#FileExists("#base#/copy.txt")#</cfoutput>

<!--- move --->
<cffile action="move" source="#base#/renamed.txt" destination="#base#/moved.txt"></cffile>
<cfoutput>|move:#FileExists("#base#/moved.txt")#:#FileExists("#base#/renamed.txt")#</cfoutput>

<!--- the cffile struct is created (empty) after a non-upload action --->
<cffile action="read" file="#base#/moved.txt" variable="c7"></cffile>
<cfoutput>|cffilestruct:#IsDefined("cffile")#:#StructKeyList(cffile)#</cfoutput>

<!--- result attribute is ignored for non-upload actions --->
<cffile action="read" file="#base#/moved.txt" variable="c8" result="myres"></cffile>
<cfoutput>|resultattr:#IsDefined("cffile")#:#IsDefined("myres")#</cfoutput>

<!--- fixnewline --->
<cffile action="write" file="#base#/w4.txt" output="A#chr(10)#B#chr(10)#C" addnewline="no" fixnewline="yes"></cffile>
<cffile action="read" file="#base#/w4.txt" variable="c9"></cffile>
<cfoutput>|fixnl:#c9#</cfoutput>

<!--- delete --->
<cffile action="delete" file="#base#/moved.txt"></cffile>
<cfoutput>|delete:#FileExists("#base#/moved.txt")#</cfoutput>

<!--- read a missing file --->
<cftry>
<cffile action="read" file="#base#/nope.txt" variable="cx"></cffile>
<cfoutput>|readmissing:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|readmissing:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- write without output or body --->
<cftry>
<cffile action="write" file="#base#/w5.txt"></cffile>
<cfoutput>|nowrite:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|nowrite:#cfcatch.type#:#cfcatch.message#:#cfcatch.detail#</cfoutput></cfcatch>
</cftry>

<!--- copy over itself --->
<cftry>
<cffile action="copy" source="#base#/w1.txt" destination="#base#/w1.txt"></cffile>
<cfoutput>|copyself:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|copyself:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- move a missing source --->
<cftry>
<cffile action="move" source="#base#/missing_src.txt" destination="#base#/dest.txt"></cffile>
<cfoutput>|movesrc:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|movesrc:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- delete a missing file --->
<cftry>
<cffile action="delete" file="#base#/missing_del.txt"></cffile>
<cfoutput>|delmissing:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|delmissing:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- write with output AND a non-blank body is rejected --->
<cftry>
<cffile action="write" file="#base#/w6.txt" output="attr">BODY</cffile>
<cfoutput>|bodyboth:NOERR</cfoutput>
<cfcatch type="any"><cfoutput>|bodyboth:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- cleanup --->
<cfdirectory action="delete" directory="#base#" recurse="true"></cfdirectory>
<cfoutput>|final:#DirectoryExists(base)#</cfoutput>

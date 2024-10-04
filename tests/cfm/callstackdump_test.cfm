<!--- CallStackDump: text representation (function frames template:FUNCTION:line, page frames template:line) appended to a file --->
<cfscript>
function inner() {
    CallStackDump(dumpPath);
}
</cfscript>
<cfset dumpPath = GetTempDirectory() & "csd_dump.txt">
<cfif FileExists(dumpPath)><cfset FileDelete(dumpPath)></cfif>
<cfset inner()>
<cfset content = FileRead(dumpPath)>
<cfset lines = ListToArray(content, Chr(10))>
<cfloop index="i" from="1" to="#arrayLen(lines)#">
<cfif Len(Trim(lines[i]))>
<cfoutput>[#i#]LN=#ListLast(lines[i], ":")#</cfoutput>
<cfif ListLen(lines[i], ":") eq 3><cfoutput>|FN=#ListGetAt(lines[i], 2, ":")#</cfoutput></cfif>
</cfif>
</cfloop>

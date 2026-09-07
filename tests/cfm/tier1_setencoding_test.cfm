<!--- Tier-1: SetEncoding + GetEncoding interplay + invalid-scope error (verified against CF 2025). --->
<cfoutput>
<cfset SetEncoding("form", "utf-8")>1:[#GetEncoding("form")#]|
<cfset SetEncoding("form", "ISO-8859-1")>2:[#GetEncoding("form")#]|
<cfset SetEncoding("url", "windows-1252")>3:[#GetEncoding("url")#]|
<cfset SetEncoding("URL", "UTF-8")>4:[#GetEncoding("url")#]|
5:<cftry><cfset SetEncoding("foo", "utf-8")><cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
</cfoutput>

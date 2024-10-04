<!--- Tier-1: GetEncoding + invalid-scope error (verified against CF 2025). --->
<cfoutput>
1:[#GetEncoding("form")#]|2:[#GetEncoding("url")#]|3:[#GetEncoding("FORM")#]|4:<cftry>#GetEncoding("foo")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
</cfoutput>

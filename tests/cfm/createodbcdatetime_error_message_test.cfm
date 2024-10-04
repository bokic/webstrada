<!--- CreateODBCDateTime invalid-date message (was BUGS.md #34) --->
<cfoutput>
1:<cftry>#CreateODBCDateTime("bad-date")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|2:<cftry>#CreateODBCDateTime("2026.13.05 10:20")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|3:<cftry>#CreateODBCDateTime("2026-13-05")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|4:<cftry>#CreateODBCDateTime("2026.05.32")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|5:<cftry>#CreateODBCDateTime("2026.05.21 25:00")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
</cfoutput>

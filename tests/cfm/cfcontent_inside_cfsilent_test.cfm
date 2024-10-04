<!--- <cfcontent> reset inside <cfsilent> clears the whole page (was BUGS.md #10) --->
<cfoutput>A|</cfoutput><cfsilent><cfcontent type="text/plain"></cfsilent><cfoutput>|B</cfoutput>

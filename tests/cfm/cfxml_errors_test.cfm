<cftry>
<cfxml variable="bad"><root><a></root></cfxml>
<cfoutput>BAD_OK</cfoutput>
<cfcatch type="any"><cfoutput>BAD:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
<cfxml variable="empty"></cfxml>
<cfoutput>EMPTY_OK</cfoutput>
<cfcatch type="any"><cfoutput>EMPTY:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
<cfxml variable="emptySelf"/>
<cfoutput>SELF_OK</cfoutput>
<cfcatch type="any"><cfoutput>SELF:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
<cfxml variable="wsOnly">   </cfxml>
<cfoutput>WSONLY_OK</cfoutput>
<cfcatch type="any"><cfoutput>WSONLY:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

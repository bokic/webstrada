<cftry>
<cfset r = XmlParse("<invalid>")>
<cfcatch type="any"><cfoutput>[#cfcatch.type#][#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
<cftry>
<cfset r = XmlParse("<a><b></a>")>
<cfcatch type="any"><cfoutput>[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

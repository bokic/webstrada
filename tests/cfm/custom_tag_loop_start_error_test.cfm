<cfimport prefix="mytag" taglib="customtags">
<cftry>
<mytag:loops>BODY</mytag:loops>
<cfcatch any><cfoutput>[caught:#cfcatch.type#:#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

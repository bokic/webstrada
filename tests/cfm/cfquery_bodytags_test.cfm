<cfset useName = true>
<cfquery name="q" datasource="webstrada" result="r">
SELECT 1 AS id
<cfif useName>
, 2 AS name
</cfif>
</cfquery>
<cfoutput>[#r.SQL#]</cfoutput>
<cfquery name="q2" datasource="webstrada" result="r2">
<cfoutput>SELECT 1 AS one</cfoutput>
</cfquery>
<cfoutput>[#r2.SQL#]</cfoutput>

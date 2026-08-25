<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_cfout
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_cfout (id INTEGER, name TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_cfout VALUES (1, 'alice')
</cfquery>

<!--- MangoBlog-style: enablecfoutputonly=true then a cfquery with cfqueryparam;
     the SQL body must be captured verbatim (CF arms the query body with
     cfoutput(true), so enablecfoutputonly does not suppress it) --->
<cfsetting enablecfoutputonly="true">
<cfquery name="q1" datasource="webstrada" result="r1">
SELECT  id, name
FROM cfq_cfout
WHERE name = <cfqueryparam value="alice" cfsqltype="CF_SQL_VARCHAR" maxlength="35"/>
AND id = <cfqueryparam value="1" cfsqltype="CF_SQL_INTEGER"/>
ORDER BY id
</cfquery>
<cfsetting enablecfoutputonly="false">
<cfoutput>A[#r1.SQL#]</cfoutput>
<cfoutput>B[#q1.recordcount#|#q1.id#|#q1.name#]</cfoutput>

<!--- query without cfqueryparam under enablecfoutputonly --->
<cfsetting enablecfoutputonly="true">
<cfquery name="q2" datasource="webstrada" result="r2">
SELECT 1 AS one
</cfquery>
<cfsetting enablecfoutputonly="false">
<cfoutput>C[#r2.SQL#|#q2.one#]</cfoutput>

<!--- enablecfoutputonly does not suppress a cfoutput inside the query body --->
<cfsetting enablecfoutputonly="true">
<cfquery name="q3" datasource="webstrada" result="r3">
<cfoutput>SELECT 2 AS two</cfoutput>
</cfquery>
<cfsetting enablecfoutputonly="false">
<cfoutput>D[#r3.SQL#|#q3.two#]</cfoutput>

<!--- cfsavecontent body remains suppressed under enablecfoutputonly (CF parity) --->
<cfsetting enablecfoutputonly="true">
<cfsavecontent variable="sc">SAVED</cfsavecontent>
<cfsetting enablecfoutputonly="false">
<cfoutput>E[#sc#]</cfoutput>

<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_cfout
</cfquery>

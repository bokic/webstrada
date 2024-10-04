<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_ms
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_ms (id INTEGER)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_ms VALUES (1)
</cfquery>
<cfquery name="q1" datasource="webstrada" result="r1">
SELECT 1 AS a; SELECT 2 AS b
</cfquery>
<cfoutput>Q1=[#q1.recordcount#][#q1.columnlist#][#r1.RECORDCOUNT#]</cfoutput>
<cfquery name="q2" datasource="webstrada" result="r2">
INSERT INTO cfq_ms (id) VALUES (2); SELECT 5 AS five
</cfquery>
<cfoutput>Q2=[#IsDefined("q2")#][#r2.RECORDCOUNT#]</cfoutput>
<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_ms
</cfquery>

<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_param
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_param (id INTEGER, name TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_param VALUES (1, 'alice')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_param VALUES (2, 'bob')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_param VALUES (3, "O'Brien")
</cfquery>

<!--- integer param binds a where clause --->
<cfset x = 2>
<cfquery name="q1" datasource="webstrada">
SELECT id, name FROM cfq_param WHERE id = <cfqueryparam value="#x#" cfsqltype="CF_SQL_INTEGER">
</cfquery>
<cfoutput>A[#q1.recordcount#|#q1.name#]</cfoutput>

<!--- inline literal value (not expression) --->
<cfquery name="q2" datasource="webstrada">
SELECT name FROM cfq_param WHERE id = <cfqueryparam value="1" cfsqltype="CF_SQL_INTEGER">
</cfquery>
<cfoutput>B[#q2.name#]</cfoutput>

<!--- string param with a quote (escaped) --->
<cfquery name="q3" datasource="webstrada">
SELECT id FROM cfq_param WHERE name = <cfqueryparam value="O'Brien" cfsqltype="CF_SQL_VARCHAR">
</cfquery>
<cfoutput>C[#q3.id#]</cfoutput>

<!--- list param expands to IN (...) --->
<cfquery name="q4" datasource="webstrada">
SELECT name FROM cfq_param WHERE id IN (<cfqueryparam value="1,2" cfsqltype="CF_SQL_INTEGER" list="true">)
</cfquery>
<cfoutput>D[#q4.recordcount#|#q4.name#]</cfoutput>

<!--- list param with custom separator --->
<cfquery name="q5" datasource="webstrada">
SELECT name FROM cfq_param WHERE id IN (<cfqueryparam value="2;3" cfsqltype="CF_SQL_INTEGER" list="true" separator=";">)
</cfquery>
<cfoutput>E[#q5.recordcount#|#q5.name#]</cfoutput>

<!--- null param --->
<cfquery name="q6" datasource="webstrada" result="r6">
SELECT <cfqueryparam value="ignored" null="true" cfsqltype="CF_SQL_VARCHAR"> AS v
</cfquery>
<cfoutput>F[#q6.v#]</cfoutput>

<!--- numeric coercion: decimal string truncates to integer --->
<cfquery name="q7" datasource="webstrada" result="r7">
SELECT <cfqueryparam value="30.9" cfsqltype="CF_SQL_INTEGER"> AS n
</cfquery>
<cfoutput>G[#q7.n#]</cfoutput>

<!--- default cfsqltype is CF_SQL_CHAR (string) --->
<cfquery name="q8" datasource="webstrada">
SELECT id FROM cfq_param WHERE name = <cfqueryparam value="alice">
</cfquery>
<cfoutput>H[#q8.id#]</cfoutput>

<!--- date / timestamp formatting --->
<cfquery name="q9" datasource="webstrada">
SELECT <cfqueryparam value="{ts '2026-08-24 23:08:57'}" cfsqltype="CF_SQL_TIMESTAMP"> AS ts_col
</cfquery>
<cfoutput>I[#q9.ts_col#]</cfoutput>


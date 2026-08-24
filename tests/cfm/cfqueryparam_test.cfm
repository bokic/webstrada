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

<!--- boolean values coerced to integer/numeric types (e.g. tinyint, smallint, integer, numeric) --->
<cfset bTrue = true>
<cfset bFalse = false>
<cfquery name="q10" datasource="webstrada">
SELECT
  <cfqueryparam value="#bTrue#" cfsqltype="cf_sql_tinyint"> AS t1,
  <cfqueryparam value="#bFalse#" cfsqltype="cf_sql_tinyint"> AS t0,
  <cfqueryparam value="true" cfsqltype="cf_sql_integer"> AS i1,
  <cfqueryparam value="false" cfsqltype="cf_sql_integer"> AS i0,
  <cfqueryparam value="yes" cfsqltype="cf_sql_smallint"> AS s1,
  <cfqueryparam value="no" cfsqltype="cf_sql_numeric"> AS n0
</cfquery>
<cfoutput>J[#q10.t1#|#q10.t0#|#q10.i1#|#q10.i0#|#q10.s1#|#q10.n0#]</cfoutput>


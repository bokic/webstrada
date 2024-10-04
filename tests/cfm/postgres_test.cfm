<!--- Requires a PostgreSQL datasource named "pgtest" (backend="postgres")
     configured on both the CF server and this engine (WSDATASOURCE_PGTEST_*). --->
<cfquery datasource="pgtest">
DROP TABLE IF EXISTS cfq_pg
</cfquery>
<cfquery datasource="pgtest">
CREATE TABLE cfq_pg (id SERIAL PRIMARY KEY, name VARCHAR(255), age INT)
</cfquery>
<cfquery datasource="pgtest">
INSERT INTO cfq_pg (name, age) VALUES ('alice', 30)
</cfquery>
<cfquery datasource="pgtest">
INSERT INTO cfq_pg (name, age) VALUES ('bob', 25)
</cfquery>

<!--- basic select --->
<cfquery name="q" datasource="pgtest">
SELECT id, name, age FROM cfq_pg ORDER BY id
</cfquery>
<cfoutput>A[#q.recordcount#|#q.name#|#q.age#]</cfoutput>

<!--- result metadata (PG has no implicit OIDs, so GENERATEDKEY is absent) --->
<cfquery name="q2" datasource="pgtest" result="r">
INSERT INTO cfq_pg (name, age) VALUES ('carol', 35)
</cfquery>
<cfoutput>B[#r.RECORDCOUNT#|#StructKeyExists(r, 'GENERATEDKEY')#]</cfoutput>

<!--- where clause --->
<cfquery name="q3" datasource="pgtest">
SELECT name FROM cfq_pg WHERE age > 30 ORDER BY id
</cfquery>
<cfoutput>C[#q3.recordcount#|#q3.name#]</cfoutput>

<!--- type mappings --->
<cfquery name="q4" datasource="pgtest">
SELECT true AS b, 3.5::numeric AS num, 'x' AS txt
</cfquery>
<cfoutput>D[#q4.b#|#q4.num#|#q4.txt#]</cfoutput>

<!--- INSERT ... RETURNING surfaces a result set --->
<cfquery name="q5" datasource="pgtest">
INSERT INTO cfq_pg (name, age) VALUES ('dave', 40) RETURNING id
</cfquery>
<cfoutput>E[#q5.recordcount#|#q5.id#]</cfoutput>

<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_inc
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_inc (id INTEGER, note TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_inc VALUES (7, 'seven')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_inc VALUES (9, 'nine')
</cfquery>
<cfset fval = 7>
<cfquery name="q1" datasource="webstrada" result="r1">
SELECT id FROM cfq_inc
<cfinclude template="include_lib/cfq_inc_sql.cfm">
</cfquery>
<cfoutput>Q1_SQL=[#r1.SQL#] Q1_RC=[#q1.recordcount#]</cfoutput>
<cfquery name="q2" datasource="webstrada" result="r2">
SELECT id FROM cfq_inc
<cfinclude template="include_lib/cfq_inc_sql_out.cfm">
</cfquery>
<cfoutput>Q2_SQL=[#r2.SQL#] Q2_RC=[#q2.recordcount#]</cfoutput>
<cftry>
<cfquery name="q3" datasource="webstrada" result="r3">
SELECT id FROM cfq_inc
<cfinclude template="include_lib/cfq_inc_sql_raw.cfm">
</cfquery>
<cfcatch type="any">
<cfoutput>Q3_ERR=[#cfcatch.message#]</cfoutput>
</cfcatch>
</cftry>

<cfinclude template="include_lib/cfq_sql_schema.sql">
<cfquery name="q" datasource="webstrada">
SELECT note FROM cfq_inc_sql
</cfquery>
<cfoutput>RC=[#q.recordcount#] NOTE=[#q.note#]</cfoutput>

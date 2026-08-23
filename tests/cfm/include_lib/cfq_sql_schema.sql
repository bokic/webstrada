<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_inc_sql
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_inc_sql (id INTEGER, note TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_inc_sql VALUES (7, 'seven')
</cfquery>

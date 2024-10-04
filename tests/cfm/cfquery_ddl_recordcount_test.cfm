<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_ddl_rec
</cfquery>
<cfquery datasource="webstrada" result="r1">
CREATE TABLE cfq_ddl_rec (id INTEGER, name TEXT)
</cfquery>
<cfquery datasource="webstrada" result="r2">
DROP TABLE IF EXISTS cfq_ddl_rec
</cfquery>
<cfquery datasource="webstrada" result="r3">
CREATE TABLE cfq_ddl_rec (id INTEGER, name TEXT)
</cfquery>
<cfquery datasource="webstrada" result="r4">
INSERT INTO cfq_ddl_rec VALUES (1, 'a')
</cfquery>
<cfquery datasource="webstrada" result="r5">
CREATE INDEX cfq_ddl_rec_idx ON cfq_ddl_rec (id)
</cfquery>
<cfquery datasource="webstrada" result="r6">
ALTER TABLE cfq_ddl_rec ADD COLUMN extra TEXT
</cfquery>
<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_ddl_rec
</cfquery>
<cfoutput>
#r1.RECORDCOUNT#|#r2.RECORDCOUNT#|#r3.RECORDCOUNT#|#r4.RECORDCOUNT#|#r5.RECORDCOUNT#|#r6.RECORDCOUNT#|
</cfoutput>

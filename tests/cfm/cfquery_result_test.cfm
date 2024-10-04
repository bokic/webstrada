<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_r
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_r (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_r (name) VALUES ('a')
</cfquery>
<cfset minAge = 0>
<cfquery name="q" datasource="webstrada" result="r">
SELECT id, name FROM cfq_r WHERE id > #minAge# ORDER BY id
</cfquery>
<cfoutput>#q.recordcount#|#r.RECORDCOUNT#|#r.CACHED#|#r.COLUMNLIST#|#r.SQL#|#StructKeyExists(r, "EXECUTIONTIME")#</cfoutput>

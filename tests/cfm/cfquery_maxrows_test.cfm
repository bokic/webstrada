<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_m
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_m (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_m (name) VALUES ('one')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_m (name) VALUES ('two')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_m (name) VALUES ('three')
</cfquery>
<cfquery name="q" datasource="webstrada" maxrows="2">
SELECT id, name FROM cfq_m ORDER BY id
</cfquery>
<cfoutput>#q.recordcount#:#q.id#-#q.name#;</cfoutput>
<cfquery name="empty" datasource="webstrada">
SELECT id, name FROM cfq_m WHERE id > 1000
</cfquery>
<cfoutput>|#empty.recordcount#|#empty.columnlist#|</cfoutput>

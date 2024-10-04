<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_loop
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_loop (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, grp TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_loop (name, grp) VALUES ('a1', 'g1')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_loop (name, grp) VALUES ('a2', 'g1')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_loop (name, grp) VALUES ('b1', 'g2')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_loop (name, grp) VALUES ('b2', 'g2')
</cfquery>
<cfquery name="q" datasource="webstrada">
SELECT id, name, grp FROM cfq_loop ORDER BY id
</cfquery>
<cfloop query="q">
<cfoutput>#q.id#:#q.name#:#q.currentrow#;</cfoutput>
</cfloop>
|UNQUAL<cfloop query="q">
<cfoutput>#name#:#currentrow#:#recordcount#:#columnlist#;</cfoutput>
</cfloop>
|AFTER<cfloop query="q">
<cfoutput>#q.id#;</cfoutput>
</cfloop>
<cfoutput>|#q.currentrow#:#q.name#</cfoutput>

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
<cfloop query="q" startrow="2" endrow="3">
<cfoutput>#q.id#:#q.name#:#q.currentrow#;</cfoutput>
</cfloop>
|EMPTY<cfloop query="q" startrow="5" endrow="6">
<cfoutput>X#q.id#</cfoutput>
</cfloop>Y|GROUP<cfloop query="q" group="grp">
<cfoutput>#q.id#:#q.name#;</cfoutput>
</cfloop>
|GROUPCS<cfloop query="q" group="grp" groupcasesensitive="true">
<cfoutput>#q.id#:#q.name#;</cfoutput>
</cfloop>

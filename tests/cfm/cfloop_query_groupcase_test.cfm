<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_grp
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_grp (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, grp TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_grp (name, grp) VALUES ('a1', 'G1')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_grp (name, grp) VALUES ('a2', 'g1')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_grp (name, grp) VALUES ('b1', 'G2')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_grp (name, grp) VALUES ('b2', 'g2')
</cfquery>
<cfquery name="q" datasource="webstrada">
SELECT id, name, grp FROM cfq_grp ORDER BY id
</cfquery>
DEFAULT<cfloop query="q" group="grp">
<cfoutput>#q.id#:#q.name#;</cfoutput>
</cfloop>
|CS<cfloop query="q" group="grp" groupcasesensitive="true">
<cfoutput>#q.id#:#q.name#;</cfoutput>
</cfloop>

<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_out
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_out (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, grp TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_out (name, grp) VALUES ('a1', 'g1')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_out (name, grp) VALUES ('a2', 'g1')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_out (name, grp) VALUES ('b1', 'g2')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_out (name, grp) VALUES ('b2', 'g2')
</cfquery>
<cfquery name="q" datasource="webstrada">
SELECT id, name, grp FROM cfq_out ORDER BY id
</cfquery>
BASIC:<cfoutput query="q">#q.id#:#q.name#:#q.currentrow#;</cfoutput>
|UNQUAL:<cfoutput query="q">#name#:#currentrow#:#recordcount#:#columnlist#;</cfoutput>
|RANGE:<cfoutput query="q" startrow="2" maxrows="2">#id#:#name#;</cfoutput>
|EMPTY:<cfoutput query="q" startrow="5" maxrows="2">X#id#</cfoutput>Y
|AFTER:<cfoutput>#q.currentrow#:#q.name#</cfoutput>
|GROUP:<cfoutput query="q" group="grp">#id#:#name#;</cfoutput>
|SCRIPT:<cfset res = ArrayNew(1)><cfoutput query="q" maxrows="2"><cfscript>ArrayAppend(res, name);</cfscript></cfoutput><cfoutput>#ArrayToList(res)#</cfoutput>

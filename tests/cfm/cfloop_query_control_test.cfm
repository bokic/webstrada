<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_loop
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_loop (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_loop (name) VALUES ('a1')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_loop (name) VALUES ('a2')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_loop (name) VALUES ('a3')
</cfquery>
<cfquery name="q" datasource="webstrada">
SELECT id, name FROM cfq_loop ORDER BY id
</cfquery>
BREAK<cfloop query="q">
<cfoutput>#q.id#;</cfoutput>
<cfif q.id EQ 2><cfbreak></cfif>
</cfloop>
|CONT<cfloop query="q">
<cfif q.id EQ 2><cfcontinue></cfif>
<cfoutput>#q.id#;</cfoutput>
</cfloop>
|DYN<cfset qn = "q">
<cfloop query="#qn#">
<cfoutput>#q.id#;</cfoutput>
</cfloop>

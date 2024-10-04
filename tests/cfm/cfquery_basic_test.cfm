<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_users
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, age INTEGER)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_users (name, age) VALUES ('alice', 30)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_users (name, age) VALUES ('bob', 25)
</cfquery>
<cfquery name="q" datasource="webstrada">
SELECT id, name, age FROM cfq_users ORDER BY id
</cfquery>
<cfoutput>#q.recordcount#|#q.columnlist#|#q.name#|#q.id#|#q.age#</cfoutput>

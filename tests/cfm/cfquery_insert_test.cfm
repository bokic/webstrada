<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_i
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_i (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_i (name) VALUES ('x')
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_i (name) VALUES ('y')
</cfquery>
<cfquery name="ins" datasource="webstrada" result="res">
INSERT INTO cfq_i (name) VALUES ('z')
</cfquery>
<cfoutput>#IsDefined("ins")#|#res.RECORDCOUNT#|#res.GENERATEDKEY#|#StructKeyExists(res, "COLUMNLIST")#</cfoutput>

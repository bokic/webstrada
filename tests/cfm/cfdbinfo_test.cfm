<cfquery datasource="webstrada">
DROP TABLE IF EXISTS dbi_people
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE dbi_people (pid INTEGER PRIMARY KEY, pname TEXT, page INTEGER)
</cfquery>
<cfquery datasource="webstrada">
CREATE INDEX dbi_pname_idx ON dbi_people (pname)
</cfquery>

<!--- tables with a pattern filter --->
<cfdbinfo type="tables" datasource="webstrada" name="t" pattern="dbi_people">
<cfoutput>A[#t.recordcount#|#t.table_name#|#t.table_type#|#t.remarks#]</cfoutput>

<!--- columns --->
<cfdbinfo type="columns" datasource="webstrada" name="c" table="dbi_people">
<cfoutput>B[#c.recordcount#|#c.column_name#|#c.type_name#|#c.is_primarykey#|#c.is_nullable#|#c.ordinal_position#|#c.column_size#|#c.decimal_digits#]</cfoutput>

<!--- index --->
<cfdbinfo type="index" datasource="webstrada" name="i" table="dbi_people">
<cfoutput>C[#i.recordcount#|#i.index_name#|#i.column_name#|#i.non_unique#|#i.type#|#i.ordinal_position#]</cfoutput>

<!--- version --->
<cfdbinfo type="version" datasource="webstrada" name="v">
<cfoutput>D[#v.recordcount#|#v.database_productname#]</cfoutput>

<!--- dbnames (empty on SQLite) --->
<cfdbinfo type="dbnames" datasource="webstrada" name="d">
<cfoutput>E[#d.recordcount#|#d.columnlist#]</cfoutput>

<!--- procedures (empty on SQLite) --->
<cfdbinfo type="procedures" datasource="webstrada" name="p">
<cfoutput>F[#p.recordcount#|#p.columnlist#]</cfoutput>

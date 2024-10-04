<cfquery datasource="webstrada">
DROP TABLE IF EXISTS sp_users
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE sp_users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)
</cfquery>
<!--- rollback to savepoint undoes the later insert --->
<cftransaction>
<cfquery datasource="webstrada">
INSERT INTO sp_users (name) VALUES ('a')
</cfquery>
<cfscript>
transactionSetSavepoint("sp1");
</cfscript>
<cfquery datasource="webstrada">
INSERT INTO sp_users (name) VALUES ('b')
</cfquery>
<cfscript>
transactionRollback("sp1");
</cfscript>
</cftransaction>
<cfquery name="q" datasource="webstrada">
SELECT name FROM sp_users
</cfquery>
<cfoutput>A:[#q.recordcount#]</cfoutput>
<!--- rollback to an unknown savepoint throws --->
<cftry>
<cftransaction>
<cfscript>
transactionRollback("nope");
</cfscript>
</cftransaction>
<cfcatch type="any">
<cfoutput>B:[#cfcatch.message#]</cfoutput>
</cfcatch>
</cftry>
<!--- savepoint outside a transaction throws --->
<cfscript>
try {
    transactionSetSavepoint("sp1");
} catch (any e) {
    writeOutput("C:[" & e.message & "]");
}
</cfscript>
<!--- multiple savepoints: rollback to the first keeps both inserts --->
<cfquery datasource="webstrada">
DROP TABLE IF EXISTS sp_users2
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE sp_users2 (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)
</cfquery>
<cftransaction>
<cfquery datasource="webstrada">
INSERT INTO sp_users2 (name) VALUES ('a')
</cfquery>
<cfscript>
transactionSetSavepoint("s1");
</cfscript>
<cfquery datasource="webstrada">
INSERT INTO sp_users2 (name) VALUES ('b')
</cfquery>
<cfscript>
transactionSetSavepoint("s2");
</cfscript>
<cfquery datasource="webstrada">
INSERT INTO sp_users2 (name) VALUES ('c')
</cfquery>
<cfscript>
transactionRollback("s2");
</cfscript>
</cftransaction>
<cfquery name="q2" datasource="webstrada">
SELECT name FROM sp_users2 ORDER BY id
</cfquery>
<cfoutput>D:[#q2.recordcount#]</cfoutput>

<!--- cftransaction commit: the INSERT survives the transaction block. --->
<cftry>
<cftransaction>
<cfquery datasource="webstrada">
DROP TABLE IF EXISTS tx_cfm
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE tx_cfm (id INTEGER PRIMARY KEY AUTOINCREMENT, name VARCHAR(50))
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO tx_cfm (name) VALUES ('kept')
</cfquery>
</cftransaction>
<cfquery name="q1" datasource="webstrada">
SELECT name FROM tx_cfm
</cfquery>
<cfoutput>commit:#q1.recordcount#|#q1.name#</cfoutput>
<cfcatch type="any"><cfoutput>ERR1:#cfcatch.message#</cfoutput></cfcatch>
</cftry>
<!--- cftransaction rollback: the INSERT is undone when the block throws. --->
<cftry>
<cftransaction>
<cfquery datasource="webstrada">
INSERT INTO tx_cfm (name) VALUES ('undone')
</cfquery>
<cfthrow message="boom">
</cftransaction>
<cfcatch type="any"><cfoutput>|rollback_caught:#cfcatch.message#</cfoutput></cfcatch>
</cftry>
<cfquery name="q2" datasource="webstrada">
SELECT name FROM tx_cfm
</cfquery>
<cfoutput>|#q2.recordcount#</cfoutput>

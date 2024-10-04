<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_colarith
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE cfq_colarith (id INTEGER)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_colarith VALUES (1)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO cfq_colarith VALUES (2)
</cfquery>
<cfquery name="cnt" datasource="webstrada">
SELECT COUNT(*) AS Total FROM cfq_colarith
</cfquery>
<cfquery name="zero" datasource="webstrada">
SELECT COUNT(*) AS Total FROM cfq_colarith WHERE id > 100
</cfquery>
<cfoutput>[#cnt.Total#]|#(cnt.Total - 0)#|#Evaluate(cnt.Total - zero.Total)#|#(cnt.Total * 2)#|#zero.Total#</cfoutput>
<cfquery datasource="webstrada">
DROP TABLE IF EXISTS cfq_colarith
</cfquery>

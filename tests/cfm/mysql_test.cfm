<cfquery datasource="mysqltest">
DROP TABLE IF EXISTS cfq_mysql
</cfquery>
<cfquery datasource="mysqltest">
CREATE TABLE cfq_mysql (id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(255), age INT)
</cfquery>
<cfquery datasource="mysqltest">
INSERT INTO cfq_mysql (name, age) VALUES ('alice', 30)
</cfquery>
<cfquery datasource="mysqltest">
INSERT INTO cfq_mysql (name, age) VALUES ('bob', 25)
</cfquery>

<!--- basic select --->
<cfquery name="q" datasource="mysqltest">
SELECT id, name, age FROM cfq_mysql ORDER BY id
</cfquery>
<cfoutput>A[#q.recordcount#|#q.name#|#q.age#]</cfoutput>

<!--- result metadata --->
<cfquery name="q2" datasource="mysqltest" result="r">
INSERT INTO cfq_mysql (name, age) VALUES ('carol', 35)
</cfquery>
<cfoutput>B[#r.RECORDCOUNT#|#r.GENERATEDKEY#]</cfoutput>

<!--- where clause --->
<cfquery name="q3" datasource="mysqltest">
SELECT name FROM cfq_mysql WHERE age > 30 ORDER BY id
</cfquery>
<cfoutput>C[#q3.recordcount#|#q3.name#]</cfoutput>

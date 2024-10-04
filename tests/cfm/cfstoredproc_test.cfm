<!--- Requires a MySQL/MariaDB datasource named "mysqltest" (backend="mysql")
     configured on both the CF server and this engine (WSDATASOURCE_MYSQLTEST_*).
     The test procedures are created defensively. Stored-procedure output cannot
     be byte-verified against CF because the RDS host has no MySQL datasource. --->
<cfquery datasource="mysqltest">
DROP PROCEDURE IF EXISTS cfq_sp_add
</cfquery>
<cfquery datasource="mysqltest">
CREATE PROCEDURE cfq_sp_add (IN a INT, IN b INT, OUT s INT)
BEGIN
SET s = a + b;
END
</cfquery>
<cfquery datasource="mysqltest">
DROP PROCEDURE IF EXISTS cfq_sp_rows
</cfquery>
<cfquery datasource="mysqltest">
CREATE PROCEDURE cfq_sp_rows (IN n INT)
BEGIN
DROP TEMPORARY TABLE IF EXISTS cfq_sp_tmp;
CREATE TEMPORARY TABLE cfq_sp_tmp (id INT, nm VARCHAR(20));
SET @i = 1;
WHILE @i <= n DO
INSERT INTO cfq_sp_tmp VALUES (@i, CONCAT('name', @i));
SET @i = @i + 1;
END WHILE;
SELECT * FROM cfq_sp_tmp ORDER BY id;
END
</cfquery>

<!--- OUT parameter + result struct --->
<cfstoredproc procedure="cfq_sp_add" datasource="mysqltest" result="r">
<cfprocparam type="in" cfsqltype="CF_SQL_INTEGER" value="2">
<cfprocparam type="in" cfsqltype="CF_SQL_INTEGER" value="3">
<cfprocparam type="out" cfsqltype="CF_SQL_INTEGER" variable="sum">
</cfstoredproc>
<cfoutput>A[#sum#|#r.CACHED#|#StructKeyExists(r, 'EXECUTIONTIME')#]</cfoutput>

<!--- returncode --->
<cfstoredproc procedure="cfq_sp_add" datasource="mysqltest" returncode="yes">
<cfprocparam type="in" cfsqltype="CF_SQL_INTEGER" value="10">
<cfprocparam type="in" cfsqltype="CF_SQL_INTEGER" value="5">
<cfprocparam type="out" cfsqltype="CF_SQL_INTEGER" variable="s2">
</cfstoredproc>
<cfoutput>B[#s2#|#cfstoredproc.statuscode#]</cfoutput>

<!--- result set binding --->
<cfstoredproc procedure="cfq_sp_rows" datasource="mysqltest">
<cfprocparam type="in" cfsqltype="CF_SQL_INTEGER" value="3">
<cfprocresult name="q" resultset="1">
</cfstoredproc>
<cfoutput>C[#q.recordcount#|#q.id#|#q.nm#]</cfoutput>

<!--- MySQL/MariaDB backend test: requires a reachable MySQL server with the
     "mysql" datasource registered (WSDATASOURCE_MYSQL_*, see run-dev.sh).
     Byte-verified against CF 2025 on a live MySQL 26.7 server except the
     multi-statement section N (CF's MySQL datasource needs
     allowMultiQueries=true in the JDBC URL, a datasource setting). Run with:
       WSDATASOURCE_MYSQL_BACKEND=mysql WSDATASOURCE_MYSQL_HOST=... \
       WSDATASOURCE_MYSQL_DATABASE=... WSDATASOURCE_MYSQL_USERNAME=... \
       WSDATASOURCE_MYSQL_PASSWORD=... bin/webstrada-cli tests/cfm/mysql_backend_test.cfm --->

<cfquery datasource="mysql">
DROP TABLE IF EXISTS mb_types
</cfquery>
<cfquery datasource="mysql">
CREATE TABLE mb_types (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(100),
  age INT,
  price DECIMAL(10,2),
  flag TINYINT(1),
  bio TEXT,
  born DATE,
  created DATETIME
)
</cfquery>

<!--- A: typed insert + select roundtrip --->
<cfquery datasource="mysql">
INSERT INTO mb_types (name, age, price, flag, bio, born, created)
VALUES ('alice', 30, 12.50, 1, 'hello world', '1995-01-15', '2020-06-01 10:30:00')
</cfquery>
<cfquery name="q" datasource="mysql">
SELECT id, name, age, price, flag, bio, born, created FROM mb_types WHERE name = 'alice'
</cfquery>
<cfoutput>A[#q.recordcount#|#q.name#|#q.age#|#q.price#|#q.flag#|#q.bio#|#q.born#]</cfoutput>

<!--- B: auto-increment GENERATEDKEY --->
<cfquery name="q2" datasource="mysql" result="r">
INSERT INTO mb_types (name, age) VALUES ('bob', 25)
</cfquery>
<cfoutput>B[#r.RECORDCOUNT#|#r.GENERATEDKEY#]</cfoutput>

<!--- C: NULL cells -> empty string --->
<cfquery name="q3" datasource="mysql">
SELECT name, age, price, bio FROM mb_types WHERE id = 2
</cfquery>
<cfoutput>C[#q3.price#|#q3.bio#]</cfoutput>

<!--- D: numeric WHERE + ordering --->
<cfquery name="q4" datasource="mysql">
SELECT name, age FROM mb_types WHERE age >= 25 ORDER BY age
</cfquery>
<cfoutput>D[#q4.recordcount#|#q4.name#|#q4.age#]</cfoutput>

<!--- E: cfqueryparam integer coercion --->
<cfquery name="q5" datasource="mysql">
SELECT name FROM mb_types WHERE age = <cfqueryparam value="30" cfsqltype="cf_sql_integer">
</cfquery>
<cfoutput>E[#q5.name#]</cfoutput>

<!--- F: cfqueryparam string with quote --->
<cfquery name="q6" datasource="mysql">
SELECT name FROM mb_types WHERE bio = <cfqueryparam value="hello world" cfsqltype="cf_sql_varchar">
</cfquery>
<cfoutput>F[#q6.recordcount#|#q6.name#]</cfoutput>

<!--- G: decimal param (scale attr matches the DECIMAL(10,2) column) --->
<cfquery name="q7" datasource="mysql">
SELECT name FROM mb_types WHERE price = <cfqueryparam value="12.50" cfsqltype="cf_sql_decimal" scale="2">
</cfquery>
<cfoutput>G[#q7.name#]</cfoutput>

<!--- H: date param --->
<cfquery name="q8" datasource="mysql">
SELECT name FROM mb_types WHERE born = <cfqueryparam value="1995-01-15" cfsqltype="cf_sql_date">
</cfquery>
<cfoutput>H[#q8.name#]</cfoutput>

<!--- H2: timestamp param with ODBC string and DateTime object --->
<cfquery name="q8b" datasource="mysql">
SELECT name FROM mb_types WHERE created = <cfqueryparam value="{ts '2020-06-01 10:30:00'}" cfsqltype="cf_sql_timestamp">
</cfquery>
<cfoutput>H2[#q8b.name#]</cfoutput>


<!--- I: transactions commit --->
<cftransaction>
<cfquery datasource="mysql">
INSERT INTO mb_types (name, age) VALUES ('carol', 40)
</cfquery>
</cftransaction>
<cfquery name="q9" datasource="mysql">
SELECT name FROM mb_types WHERE name = 'carol'
</cfquery>
<cfoutput>I[#q9.recordcount#|#q9.name#]</cfoutput>

<!--- J: transaction rollback --->
<cftry>
<cftransaction>
<cfquery datasource="mysql">
INSERT INTO mb_types (name, age) VALUES ('dave', 50)
</cfquery>
<cfthrow message="force rollback">
</cftransaction>
<cfcatch type="any"></cfcatch>
</cftry>
<cfquery name="q10" datasource="mysql">
SELECT name FROM mb_types WHERE name = 'dave'
</cfquery>
<cfoutput>J[#q10.recordcount#]</cfoutput>

<!--- K: update --->
<cfquery datasource="mysql">
UPDATE mb_types SET name = 'ALICE' WHERE name = 'alice'
</cfquery>
<cfquery name="q11" datasource="mysql">
SELECT name FROM mb_types WHERE name = 'ALICE'
</cfquery>
<cfoutput>K[#q11.recordcount#|#q11.name#]</cfoutput>

<!--- L: delete --->
<cfquery datasource="mysql">
DELETE FROM mb_types WHERE name = 'carol'
</cfquery>
<cfquery name="q12" datasource="mysql">
SELECT name FROM mb_types WHERE name = 'carol'
</cfquery>
<cfoutput>L[#q12.recordcount#]</cfoutput>

<!--- M: maxrows --->
<cfquery name="q13" datasource="mysql" maxrows="1">
SELECT id FROM mb_types ORDER BY id
</cfquery>
<cfoutput>M[#q13.recordcount#|#q13.id#]</cfoutput>

<!--- N: multi-statement script, first result surfaced (both run) --->
<cfquery datasource="mysql">
INSERT INTO mb_types (name, age) VALUES ('erin', 22)
</cfquery>
<cfquery name="q14" datasource="mysql">
SELECT id, name FROM mb_types WHERE name = 'erin';
SELECT id FROM mb_types WHERE name = 'bob'
</cfquery>
<cfoutput>N[#q14.recordcount#|#q14.name#]</cfoutput>

<!--- O: LIKE + concatenated text --->
<cfquery name="q15" datasource="mysql">
SELECT name FROM mb_types WHERE name LIKE <cfqueryparam value="%al%" cfsqltype="cf_sql_varchar">
</cfquery>
<cfoutput>O[#q15.recordcount#|#q15.name#]</cfoutput>

<!--- P: cfinsert through FORM scope --->
<cfset form.name = "frank">
<cfset form.age = "35">
<cfinsert datasource="mysql" tablename="mb_types" formfields="name,age">
<cfquery name="q16" datasource="mysql">
SELECT name FROM mb_types WHERE name = 'frank'
</cfquery>
<cfoutput>P[#q16.recordcount#|#q16.name#]</cfoutput>

<!--- Q: count aggregate --->
<cfquery name="q17" datasource="mysql">
SELECT COUNT(*) AS total FROM mb_types
</cfquery>
<cfoutput>Q[#q17.total#]</cfoutput>

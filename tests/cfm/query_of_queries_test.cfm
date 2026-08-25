<cfset users = QueryNew("id,Name,age,active", "integer,varchar,integer,bit")>
<cfset QueryAddRow(users)><cfset QuerySetCell(users, "id", 1)><cfset QuerySetCell(users, "Name", "alice")><cfset QuerySetCell(users, "age", 30)><cfset QuerySetCell(users, "active", 1)>
<cfset QueryAddRow(users)><cfset QuerySetCell(users, "id", 2)><cfset QuerySetCell(users, "Name", "bob")><cfset QuerySetCell(users, "age", 25)><cfset QuerySetCell(users, "active", 1)>
<cfset QueryAddRow(users)><cfset QuerySetCell(users, "id", 3)><cfset QuerySetCell(users, "Name", "carol")><cfset QuerySetCell(users, "age", 35)><cfset QuerySetCell(users, "active", 0)>
<cfset QueryAddRow(users)><cfset QuerySetCell(users, "id", 4)><cfset QuerySetCell(users, "Name", "dave")><cfset QuerySetCell(users, "age", 28)><cfset QuerySetCell(users, "active", 0)>

<cfset orders = QueryNew("id,user_id,total", "integer,integer,double")>
<cfset QueryAddRow(orders)><cfset QuerySetCell(orders, "id", 1)><cfset QuerySetCell(orders, "user_id", 1)><cfset QuerySetCell(orders, "total", 10.5)>
<cfset QueryAddRow(orders)><cfset QuerySetCell(orders, "id", 2)><cfset QuerySetCell(orders, "user_id", 1)><cfset QuerySetCell(orders, "total", 20)>
<cfset QueryAddRow(orders)><cfset QuerySetCell(orders, "id", 3)><cfset QuerySetCell(orders, "user_id", 2)><cfset QuerySetCell(orders, "total", 5)>
<cfset QueryAddRow(orders)><cfset QuerySetCell(orders, "id", 4)><cfset QuerySetCell(orders, "user_id", 3)><cfset QuerySetCell(orders, "total", 99)>

<cfoutput>T1:</cfoutput>
<cfquery name="q" dbtype="query">SELECT id, Name FROM users WHERE age > 25 ORDER BY id</cfquery>
<cfoutput>#q.recordcount#|#q.columnlist#|#q.Name#</cfoutput>

<cfoutput>|T2:</cfoutput>
<cfquery name="q" dbtype="query">SELECT id FROM users WHERE age >= 28 AND age <= 35 ORDER BY age DESC</cfquery>
<cfoutput>#q.recordcount#|#q.id#</cfoutput>

<cfoutput>|T3:</cfoutput>
<cfquery name="q" dbtype="query">SELECT * FROM users WHERE active = 1 ORDER BY id</cfquery>
<cfoutput>#q.recordcount#|#q.Name#</cfoutput>

<cfoutput>|T4:</cfoutput>
<cfquery name="q" dbtype="query">SELECT users.id, users.Name, orders.total FROM users, orders WHERE users.id = orders.user_id ORDER BY orders.id</cfquery>
<cfoutput>#q.recordcount#|#q.Name#:#q.total#</cfoutput>

<cfoutput>|T5:</cfoutput>
<cfquery name="q" dbtype="query">SELECT users.Name, SUM(orders.total) AS total_spent FROM users, orders WHERE users.id = orders.user_id GROUP BY users.Name ORDER BY users.Name</cfquery>
<cfoutput>#q.recordcount#|#q.Name#:#q.total_spent#</cfoutput>

<cfoutput>|T6:</cfoutput>
<cfquery name="q" dbtype="query">SELECT COUNT(*) AS n, MAX(total) AS mx, MIN(total) AS mn, AVG(total) AS av FROM orders</cfquery>
<cfoutput>#q.n#|#q.mx#|#q.mn#|#q.av#</cfoutput>

<cfoutput>|T7:</cfoutput>
<cfquery name="q" dbtype="query">SELECT DISTINCT user_id FROM orders ORDER BY user_id</cfquery>
<cfoutput>#q.recordcount#|#q.user_id#</cfoutput>

<cfoutput>|T8:</cfoutput>
<cfquery name="q" dbtype="query" maxrows="2">SELECT id FROM users ORDER BY id</cfquery>
<cfoutput>#q.recordcount#|#q.id#</cfoutput>

<cfoutput>|T9:</cfoutput>
<cfquery name="q" dbtype="query">SELECT id, Name, 'Mr. ' + Name AS title FROM users WHERE id = 2</cfquery>
<cfoutput>#q.title#</cfoutput>

<cfoutput>|T10:</cfoutput>
<cfquery name="q" dbtype="query">SELECT total + 1 AS t FROM orders WHERE user_id = 1 ORDER BY id</cfquery>
<cfoutput>#q.recordcount#|#q.t#</cfoutput>

<cfoutput>|T11:</cfoutput>
<cfquery name="q" dbtype="query" result="r">SELECT id FROM USERS WHERE id = 1</cfquery>
<cfoutput>#q.recordcount#|#r.RECORDCOUNT#|#r.COLUMNLIST#|#r.CACHED#|#r.SQL#</cfoutput>

<cfoutput>|T12:</cfoutput>
<cfquery name="q" dbtype="query">SELECT Name FROM users WHERE id = <cfqueryparam value="2" cfsqltype="cf_sql_integer"></cfquery>
<cfoutput>#q.Name#</cfoutput>

<cfoutput>|T13:</cfoutput>
<cfset r = queryExecute("SELECT Name FROM users WHERE age > 27", {}, {dbtype="query"})>
<cfoutput>#r.recordcount#|#r.Name#</cfoutput>

<cfoutput>|T14:</cfoutput>
<cfquery name="q" dbtype="query">SELECT Name FROM users WHERE Name = 'carol' OR Name = 'bob' ORDER BY Name</cfquery>
<cfoutput>#q.Name#</cfoutput>

<cfoutput>|T15:</cfoutput>
<cfquery name="q" dbtype="query">SELECT user_id, COUNT(*) AS n FROM orders GROUP BY user_id HAVING COUNT(*) > 1</cfquery>
<cfoutput>#q.recordcount#|#q.user_id#|#q.n#</cfoutput>

<cfoutput>|T16:</cfoutput>
<cfquery name="q" dbtype="query">SELECT id, total FROM orders WHERE total > 10 ORDER BY total DESC</cfquery>
<cfoutput>#q.recordcount#|#q.total#</cfoutput>

<cfoutput>|T17:</cfoutput>
<cfquery name="q" dbtype="query">SELECT id, Name FROM users WHERE age > 1000</cfquery>
<cfoutput>#q.recordcount#|#q.columnlist#</cfoutput>

<cfoutput>|T18:</cfoutput>
<cfquery name="q" dbtype="query">SELECT Name + '!' AS n1, 'Mr. ' + Name AS n2 FROM users WHERE id = 1</cfquery>
<cfoutput>#q.n1#|#q.n2#</cfoutput>

<cfoutput>|T19:</cfoutput>
<cfset events = QueryNew("id,dt", "integer,date")>
<cfset QueryAddRow(events)><cfset QuerySetCell(events, "id", 1)><cfset QuerySetCell(events, "dt", CreateDate(2024,1,15))>
<cfset QueryAddRow(events)><cfset QuerySetCell(events, "id", 2)><cfset QuerySetCell(events, "dt", CreateDate(2024,6,1))>
<cfset QueryAddRow(events)><cfset QuerySetCell(events, "id", 3)>
<cfquery name="q" dbtype="query">SELECT id FROM events WHERE dt > '#DateFormat(CreateDate(2024,3,1),'yyyy-mm-dd')#' ORDER BY id</cfquery>
<cfoutput>#q.recordcount#|#q.id#</cfoutput>

<cfoutput>|T20:</cfoutput>
<cfquery name="q" dbtype="query">SELECT id FROM events WHERE dt IS NULL</cfquery>
<cfoutput>#q.recordcount#|#q.id#</cfoutput>

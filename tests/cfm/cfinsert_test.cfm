<cfquery datasource="webstrada">
DROP TABLE IF EXISTS ci_users
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE ci_users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, age INTEGER)
</cfquery>

<!--- cfinsert from all form fields --->
<cfset form.name = "alice">
<cfset form.age = "30">
<cfinsert datasource="webstrada" tablename="ci_users">
<cfquery name="q1" datasource="webstrada">SELECT id, name, age FROM ci_users</cfquery>
<cfoutput>A[#q1.recordcount#|#q1.name#|#q1.age#]</cfoutput>

<!--- cfinsert with formfields subset (extra form field ignored) --->
<cfset form.name = "bob">
<cfset form.age = "25">
<cfset form.extra = "ignored">
<cfinsert datasource="webstrada" tablename="ci_users" formfields="name,age">
<cfquery name="q2" datasource="webstrada">SELECT name, age FROM ci_users WHERE name = 'bob'</cfquery>
<cfoutput>B[#q2.name#|#q2.age#]</cfoutput>

<!--- cfinsert with explicit pk in formfields --->
<cfset form.id = "99">
<cfset form.name = "carol">
<cfset form.age = "40">
<cfinsert datasource="webstrada" tablename="ci_users" formfields="name,age,id">
<cfquery name="q3" datasource="webstrada">SELECT id, name, age FROM ci_users WHERE id = 99</cfquery>
<cfoutput>C[#q3.id#|#q3.name#|#q3.age#]</cfoutput>

<!--- string with quote --->
<cfset form.name = "O'Reilly">
<cfset form.age = "41">
<cfinsert datasource="webstrada" tablename="ci_users" formfields="name,age">
<cfquery name="q4" datasource="webstrada">SELECT name FROM ci_users WHERE age = 41</cfquery>
<cfoutput>D[#q4.name#]</cfoutput>

<!--- cfupdate: change row 1 --->
<cfset form.id = "1">
<cfset form.name = "ALICE">
<cfset form.age = "31">
<cfset form.extra = "">
<cfupdate datasource="webstrada" tablename="ci_users" formfields="id,name,age">
<cfquery name="q5" datasource="webstrada">SELECT name, age FROM ci_users WHERE id = 1</cfquery>
<cfoutput>E[#q5.name#|#q5.age#]</cfoutput>

<!--- cfupdate with formfields (pk still required in form) --->
<cfset form.id = "2">
<cfset form.name = "BOB">
<cfset form.age = "26">
<cfset form.extra = "">
<cfupdate datasource="webstrada" tablename="ci_users" formfields="id,name,age">
<cfquery name="q6" datasource="webstrada">SELECT name, age FROM ci_users WHERE id = 2</cfquery>
<cfoutput>F[#q6.name#|#q6.age#]</cfoutput>

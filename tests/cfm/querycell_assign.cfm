<cfset q = QueryNew("a,b", "varchar,integer", [{a:"x",b:1},{a:"y",b:2},{a:"z",b:3}])>
<cfset q["a"][2] = "new">
<cfset q.b[1] = 42>
<cfset q["a"][1] = "first">
<cfoutput>#q.a[1]#|#q.a[2]#|#q.a[3]#|#q.b[1]#|#q.b[2]#|#q.b[3]#|</cfoutput>

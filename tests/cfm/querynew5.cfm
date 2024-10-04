<cfset q1 = QueryNew("a", "integer", [{a:"30.9"},{a:5},{a:7.7}])>
<cfset q2 = QueryNew("b", "bigint", [{b:"9007199254740993"}])>
<cfset q3 = QueryNew("c", "double", [{c:"1.5"}])>
<cfset q4 = QueryNew("d", "varchar", [{d:42}])>
<cfset q5 = QueryNew("a,b", "varchar,varchar", [{a:"x",b:"y"}])>
<cfset q6 = QueryNew("", "", [])>
<cfoutput>#q1.a[1]#|#q1.a[2]#|#q1.a[3]#|
#q2.b[1]#|
#q3.c[1]#|
#q4.d[1]#|
#q5.recordcount#|#q5.columnlist#|
#q6.columnlist#|#q6.recordcount#|</cfoutput>

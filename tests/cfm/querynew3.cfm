<cfset q1 = QueryNew("a,b", "varchar,varchar", [{a:"x",b:"y"},{a:"p",b:"q"}])>
<cfset q2 = QueryNew("name,age", "varchar,integer", [{name:"bob",age:30},{name:"x"}])>
<cfset q3 = QueryNew("score", "double", [{score:1.5}])>
<cfset q4 = QueryNew("big", "bigint", [{big:2147483648}])>
<cfset q6 = QueryNew("a", "varchar", [{a:"x"}])>
<cfoutput>#serializeJSON(q1)#|
#serializeJSON(q2)#|
#serializeJSON(q3)#|
#serializeJSON(q4)#|
#serializeJSON(q6)#|</cfoutput>

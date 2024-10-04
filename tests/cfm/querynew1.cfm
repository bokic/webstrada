<cfset q1 = QueryNew("a,b,c")>
<cfset q2 = QueryNew("name,age", "varchar,integer")>
<cfset q3 = QueryNew("a,b", "varchar,varchar", [{a:"x",b:"y"}])>
<cfset q4 = QueryNew("name,age", "varchar,integer", [{name:"bob",age:30},{name:"alice",age:25}])>
<cfset q5 = QueryNew("")>
<cfset q6 = QueryNew("a,b", "varchar,varchar", [[1,"two"],[4,"five"]])>
<cfset q7 = QueryNew({a:"integer", b:"varchar"})>
<cfset q8 = QueryNew("score", "double", [{score:1.5}])>
<cfset q9 = QueryNew("cnt", "bigint", [{cnt:9007199254740993}])>
<cfoutput>#q1.columnlist#|#q1.recordcount#|#isQuery(q1)#|
#q2.columnlist#|#q2.recordcount#|
#q3.columnlist#|#q3.recordcount#|#q3.a[1]#|#q3.b[1]#|
#q4.columnlist#|#q4.recordcount#|#q4.name[1]#|#q4.age[2]#|
#q5.columnlist#|#q5.recordcount#|
#q6.columnlist#|#q6.recordcount#|#q6.a[1]#|#q6.a[2]#|#q6.b[2]#|
#q7.columnlist#|#q7.recordcount#|#q7.a[1]#|#q7.b[1]#|
#q8.columnlist#|#q8.recordcount#|#q8.score[1]#|
#q9.columnlist#|#q9.recordcount#|#q9.cnt[1]#|</cfoutput>

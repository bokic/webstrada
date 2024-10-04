<cfset a = ArrayNew(2)>
<cfset a[1][5] = 1>
<cfset a[2][3] = "x">
<cfoutput>[#a[1][5]#][#a[2][3]#][#ArrayLen(a)#][#ArrayLen(a[1])#]</cfoutput>
<cfset q = QueryNew("n", "integer", [{n:1}])>
<cfset q["n"][1] += 10>
<cfoutput>|[#q.n[1]#]</cfoutput>

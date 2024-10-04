<cfset q = QueryNew("a,b", "varchar,integer")>
<cfset QueryAddRow(q)>
<cfset s1 = QuerySetCell(q, "a", "x")>
<cfset s2 = QuerySetCell(q, "b", 5)>
<cfset QueryAddRow(q)>
<cfset s3 = QuerySetCell(q, "a", "y", 2)>
<cfset QueryAddRow(q)>
<cfset s4 = QuerySetCell(q, "a", "z", -1)>
<cfset QuerySetCell(q, "b", "30.9")>
<cfoutput>#q.a[1]#|#q.b[1]#|#q.a[2]#|#q.b[2]#|#q.a[3]#|s1:#s1#|s2:#s2#|s3:#s3#|s4:#s4#|rc:#q.recordcount#|
</cfoutput>

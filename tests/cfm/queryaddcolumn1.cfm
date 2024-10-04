<cfset q = QueryNew("a")>
<cfset QueryAddRow(q)>
<cfset QueryAddRow(q)>
<cfset QueryAddRow(q)>
<cfset QuerySetCell(q, "a", "x", 1)>
<cfset r1 = QueryAddColumn(q, "b", ["p","q","r"])>
<cfset r2 = QueryAddColumn(q, "n", "integer", ["1","2","3"])>
<cfset r3 = QueryAddColumn(q, "short", ["only1"])>
<cfset r4 = QueryAddColumn(q, "long", ["1","2","3","4","5"])>
<cfoutput>rc:#q.recordcount#|cl:#q.columnlist#|
a1:#q.a[1]#|a4:#q.a[4]#|a5:#q.a[5]#|
b1:#q.b[1]#|b3:#q.b[3]#|
n1:#q.n[1]#|n2:#q.n[2]#|n3:#q.n[3]#|
s1:#q.short[1]#|s2:#q.short[2]#|s3:#q.short[3]#|
l1:#q.long[1]#|l4:#q.long[4]#|l5:#q.long[5]#|
r1:#r1#|r2:#r2#|r3:#r3#|r4:#r4#|
</cfoutput>

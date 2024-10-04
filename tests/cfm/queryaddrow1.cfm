<cfset q = QueryNew("a,b")>
<cfset r1 = QueryAddRow(q)>
<cfset r2 = QueryAddRow(q)>
<cfset r3 = QueryAddRow(q, 2)>
<cfset r4 = QueryAddRow(q, {a:"x", b:1})>
<cfset r5 = QueryAddRow(q, [{a:"p", b:2},{a:"q", b:3}])>
<cfoutput>rc:#q.recordcount#|r1:#r1#|r2:#r2#|r3:#r3#|r4:#r4#|r5:#r5#|
a1:#q.a[1]#|a5:#q.a[5]#|a6:#q.a[6]#|b6:#q.b[6]#|
</cfoutput>

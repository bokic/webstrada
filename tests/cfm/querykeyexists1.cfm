<cfset q = QueryNew("a,b", "varchar,integer", [{a:"x",b:1},{a:"y",b:2}])>
<cfoutput>k1:#QueryKeyExists(q, "a")#|k2:#QueryKeyExists(q, "B")#|k3:#QueryKeyExists(q, "zzz")#|k4:#QueryKeyExists(q, "")#|
vl1:[#ValueList(q.a)#]|vl2:[#ValueList(q.b, ";")#]|
</cfoutput>

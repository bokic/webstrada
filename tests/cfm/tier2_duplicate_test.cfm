<cfset s = {a:1, b:{c:2}, d:[1,2,3], e:'x'}>
<cfset d = Duplicate(s)>
<cfset d.b.c = 99>
<cfset d.d[1] = 50>
<cfoutput>
1:[#s.b.c#:#s.d[1]#]|2:[#d.b.c#:#d.d[1]#]|3:[#IsStruct(d)#]|4:[#IsArray(d.d)#]
</cfoutput>
<cfset q = QueryNew("col1,col2")>
<cfset QueryAddRow(q, 2)>
<cfset QuerySetCell(q, "col1", "x", 1)>
<cfset QuerySetCell(q, "col2", "y", 1)>
<cfset QuerySetCell(q, "col1", "a", 2)>
<cfset dq = Duplicate(q)>
<cfset QuerySetCell(dq, "col1", "Z", 1)>
<cfoutput>|5:[#q.col1[1]#:#dq.col1[1]#]|6:[#dq.recordcount#]</cfoutput>
<cfset a = [1,2,3]>
<cfset da = Duplicate(a)>
<cfset da[1] = 99>
<cfoutput>|7:[#a[1]#:#da[1]#]</cfoutput>
<cfoutput>|8:[#Duplicate(5)#]|9:[#Duplicate('str')#]|10:[#Duplicate(true)#]</cfoutput>

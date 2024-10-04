<cfset q = QueryNew("name,age", "varchar,integer")>
<cfset QueryAddRow(q)>
<cfset QuerySetCell(q, "name", "bob")>
<cfset QuerySetCell(q, "age", 30)>
<cfset QueryAddRow(q, {name:"alice", age:25})>
<cfset QueryAddRow(q, [{name:"carol", age:22},{name:"dave", age:41}])>
<cfset QueryAddColumn(q, "score", "double", [1.5,2.5,3.5,4.5])>
<cfset row = QueryGetRow(q, 3)>
<cfoutput>rc:#q.recordcount#|cl:#q.columnlist#|
n1:#q.name[1]#|a4:#q.age[4]#|
r3n:#row.name#|r3a:#row.age#|
ke:#QueryKeyExists(q, "score")#|
vl:[#ValueList(q.name)#]|vla:[#ValueList(q.age, "-")#]|
</cfoutput>

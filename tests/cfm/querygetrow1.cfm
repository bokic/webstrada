<cfset q = QueryNew("Name,AGE", "varchar,integer", [{Name:"x",AGE:5},{Name:"y",AGE:6}])>
<cfset row = QueryGetRow(q, 1)>
<cfset row2 = QueryGetRow(q, 2)>
<cfoutput>isStruct:#isStruct(row)#|size:#structCount(row)#|
r1:#row.Name#|r1a:#row.AGE#|r1b:#row["name"]#|
r2:#row2.Name#|r2a:#row2.AGE#|
</cfoutput>

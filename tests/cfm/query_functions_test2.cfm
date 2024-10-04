<cfset q = QueryNew("id,name", "integer,varchar", [{id:1,name:"a"},{id:2,name:"b"},{id:3,name:"c"}])>
<cfset byId = QueryFilter(q, function(row, rn, qq) { return rn EQ 2; })>
<cfoutput>#ValueList(byId.id)#|#byId.name[1]#</cfoutput>
<cfset QueryEach(q, function(row, rn, qq) { })>
<cfoutput>DONE</cfoutput>
<cfset mapped = QueryMap(q, function(row, rn, qq) { row.id = rn; row.name = qq.name[rn]; return row; })>
<cfoutput>#ValueList(mapped.id)#|#ValueList(mapped.name)#</cfoutput>
<cfset nested = QueryMap(q, function(row, rn, qq) { return {id: row.id, name: row.name & row.id}; })>
<cfoutput>#ValueList(nested.name)#</cfoutput>
<cfset reduced = QueryReduce(q, function(acc, row, rn, qq) { return acc & "[" & row.id & row.name & "]"; }, "")>
<cfoutput>#reduced#</cfoutput>
<cfset total = QueryReduce(q, function(acc, row, rn, qq) { return acc + row.id; }, 0)>
<cfoutput>#total#</cfoutput>
<cfset grid = QueryConvertForGrid(q, 2, 2)>
<cfoutput>#grid.TOTALROWCOUNT#|#ValueList(grid.QUERY.name)#</cfoutput>

<cfset q = QueryNew("id,name,age", "integer,varchar,integer", [{id:1,name:"a",age:10},{id:2,name:"b",age:20},{id:3,name:"c",age:30}])>
<cfoutput>#QuotedValueList(q.id)#</cfoutput>
<cfoutput>#QuotedValueList(q.name, "|")#</cfoutput>
<cfoutput>[#QuotedValueList(q.name, "")#]</cfoutput>
<cfset qe = QueryNew("x", "varchar", [])>
<cfoutput>[#QuotedValueList(qe.x)#]</cfoutput>
<cfset eachOut = "">
<cfset QueryEach(q, function(row, rn, qq) { eachOut = eachOut & rn & ":" & row.id & row.name & "|"; })>
<cfoutput>#eachOut#</cfoutput>
<cfset fq = QueryFilter(q, function(row, rn, qq) { return row.id MOD 2 EQ 1; })>
<cfoutput>#fq.recordcount#|#ValueList(fq.id)#|#fq.columnlist#</cfoutput>
<cfoutput>#q.recordcount#|#ValueList(q.id)#</cfoutput>
<cfset mq = QueryMap(q, function(row, rn, qq) { row.name = row.name & rn; return row; })>
<cfoutput>#mq.recordcount#|#ValueList(mq.name)#|#mq.columnlist#</cfoutput>
<cfset mq2 = QueryMap(q, function(row, rn, qq) { return {id: rn, name: "z", age: 0}; })>
<cfoutput>#ValueList(mq2.id)#|#ValueList(mq2.name)#</cfoutput>
<cfset redSum = QueryReduce(q, function(acc, row, rn, qq) { return acc + row.id; }, 0)>
<cfset redJoin = QueryReduce(q, function(acc, row, rn, qq) { return acc & row.name; }, ">")>
<cfoutput>#redSum#|#redJoin#</cfoutput>
<cfset qe2 = QueryNew("id", "integer", [])>
<cfset redEmpty = QueryReduce(qe2, function(acc, row, rn, qq) { return acc + row.id; }, 100)>
<cfoutput>#redEmpty#</cfoutput>
<cfoutput>#IsNull(QueryGetResult(q))#</cfoutput>
<cfset g1 = QueryConvertForGrid(q, 1, 2)>
<cfset g2 = QueryConvertForGrid(q, 2, 2)>
<cfset g3 = QueryConvertForGrid(q, 3, 2)>
<cfset g4 = QueryConvertForGrid(q, 10, 2)>
<cfset g5 = QueryConvertForGrid(q, 1, 0)>
<cfoutput>#ValueList(g1.QUERY.id)#:#g1.TOTALROWCOUNT#</cfoutput>
<cfoutput>#ValueList(g2.QUERY.id)#:#g2.TOTALROWCOUNT#</cfoutput>
<cfoutput>#ValueList(g3.QUERY.id)#:#g3.TOTALROWCOUNT#</cfoutput>
<cfoutput>[#ValueList(g4.QUERY.id)#]:#g4.TOTALROWCOUNT#</cfoutput>
<cfoutput>[#ValueList(g5.QUERY.id)#]:#g5.TOTALROWCOUNT#</cfoutput>

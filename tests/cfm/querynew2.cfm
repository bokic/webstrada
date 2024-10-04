<cfset q = QueryNew("name,age", "varchar,integer", [{name:"bob",age:30},{name:"alice",age:25},{name:"carol",age:22}])>
<cfoutput>#q.recordcount#|#q.columnlist#|#q.currentrow#|
#q.name[1]#|#q.name[2]#|#q.name[3]#|
#q.age[1]#|#q.age[2]#|#q.age[3]#|
#q["name"][1]#|#q["name"][2]#|#q["age"][3]#|
#q["NAME"][1]#|#q["Age"][2]#|
</cfoutput>

<cfset s = {k: ["hello", "world"]}>
<cfset arr = [3, 1, 2]>
<cfset st = {x: 10, y: 20}>
<cfset dt = createDateTime(2024, 5, 6, 7, 8, 9)>
<cfset q = queryNew("a,b")>
<cfset q.addRow()>
<cfset querySetCell(q, "a", 1, 1)>
<cfset querySetCell(q, "b", 2, 1)>
<cfoutput>#s.k[2].toUpperCase()#|#s.k[2].len()#|#s.k[2].mid(1, 3)#|#s.k[1].toLowerCase()#<br></cfoutput>
<cfoutput>#arr.len()#|#arr.max()#|#arr.min()#|#arr.sum()#|#arr.toList()#|#arr.contains(2)#<br></cfoutput>
<cfoutput>#st.keyExists("x")#|#st.count()#|#st.isEmpty()#|#st.keyList()#|#st.find("y")#<br></cfoutput>
<cfoutput>#dt.year()#|#dt.month()#|#dt.day()#|#dt.dateFormat("yyyy-mm-dd")#|#dt.datePart("m")#<br></cfoutput>
<cfoutput>#q.recordCount#|#q.columnList#|#q.len()#<br></cfoutput>

<cfset q1 = QueryNew("a,b", "varchar,varchar", [{a:"x",b:"y"},{a:"p",b:"q"}])>
<cfset q2 = QueryNew("Name,AGE", "varchar,varchar", [{Name:"x",AGE:5}])>
<cfset q3 = QueryNew("a,b")>
<cfdump var="#q1#">
<cfdump var="#q2#" format="text">
<cfdump var="#q3#">
<cfdump var="#q3#" format="text">

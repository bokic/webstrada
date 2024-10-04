<cfset q = QueryNew("a,b", "integer,varchar", [{a:1,b:"x"},{a:2},{a:3,b:"z"}])>
<cfset qe = QueryNew("a")>
<cfoutput>vl1:[#ValueList(q.a)#]|vl2:[#ValueList(q.b, "|")#]|vle:[#ValueList(qe.a)#]|
</cfoutput>

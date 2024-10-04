<cfset q = QueryNew("a,b,c,d", "varchar,integer,double,bigint", [{a:"x",b:1,c:1.5,d:9007199254740993},{a:"y",b:2,c:2.5,d:9007199254740994},{a:"z",b:3,c:3.5,d:9007199254740995}])>

<!--- Bracket-access column copies keep a writable query-column reference:
     x = q["a"]; x[1] = "MUT" writes through to the query. --->
<cfset x = q["a"]>
<cfset x[1] = "MUT">
<cfset x[2] = "M2">
<cfoutput>BRK:W#x[1]#|#x[2]#|#x[3]#|#q.a[1]#|#q.a[2]#|#q.a[3]#|</cfoutput>

<!--- Dot-access column copies are read-only: x = q.a; x[1] = "MUT" throws
     an Expression error with the cell's Java type. --->
<cftry>
  <cfset y = q.a>
  <cfset y[1] = "MUT">
  <cfoutput>DOT_OK</cfoutput>
<cfcatch><cfoutput>DOT_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- The Java type in the message follows the column type. --->
<cftry>
  <cfset y = q.b>
  <cfset y[1] = 99>
  <cfoutput>IN_OK</cfoutput>
<cfcatch><cfoutput>IN_ERR:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset y = q.c>
  <cfset y[1] = 9>
  <cfoutput>DB_OK</cfoutput>
<cfcatch><cfoutput>DB_ERR:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset y = q.d>
  <cfset y[1] = 9>
  <cfoutput>BI_OK</cfoutput>
<cfcatch><cfoutput>BI_ERR:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- Reads from a copied column work (snapshot of the column at copy time). --->
<cfset r = q["a"]>
<cfoutput>READ:#r[1]#|#r[2]#|#r[3]#|</cfoutput>

<!--- The direct forms still persist the write, matching CF. --->
<cfset q["a"][1] = "D1">
<cfset q.b[2] = 22>
<cfset q.c[3] = 3.75>
<cfset q["d"][3] = 9007199254740996>
<cfoutput>DIR:#q.a[1]#|#q.a[2]#|#q.a[3]#|#q.b[1]#|#q.b[2]#|#q.b[3]#|#q.c[1]#|#q.c[3]#|#q.d[1]#|#q.d[3]#|</cfoutput>

<!--- Same semantics from cfscript. --->
<cfscript>
qs = QueryNew("a", "varchar", [{a:"x"},{a:"y"}]);
bx = qs["a"];
bx[1] = "MUT";
writeOutput("SCR_BRK:" & bx[1] & "|" & qs.a[1] & "|");
try {
    dy = qs.a;
    dy[1] = "MUT";
    writeOutput("SCR_DOT_OK");
} catch (any e) {
    writeOutput("SCR_DOT_ERR:" & e.type & "|" & e.message & "|");
}
</cfscript>

<cfset q2 = QueryNew("a", "varchar", [{a:"x"},{a:"y"},{a:"z"}])>

<!--- A second copy of a bracket column degrades to the scalar first cell, so
     writing an element throws CF's dereference Expression error. --->
<cftry>
  <cfset x = q2["a"]>
  <cfset y = x>
  <cfset y[1] = "CPY">
  <cfoutput>CPY2_OK</cfoutput>
<cfcatch><cfoutput>CPY2_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- A second copy of a dot column throws the same way. --->
<cftry>
  <cfset d = q2.a>
  <cfset e = d>
  <cfset e[1] = "CPY">
  <cfoutput>CPY2D_OK</cfoutput>
<cfcatch><cfoutput>CPY2D_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- A stored column reference is not a real Array: ArrayLen/ArrayAppend
     reject it, IsArray is NO, while the direct bracket temp still counts. --->
<cftry>
  <cfset x = q2["a"]>
  <cfoutput>LEN1:#ArrayLen(q2["a"])#|</cfoutput>
<cfcatch><cfoutput>LEN1_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset x = q2["a"]>
  <cfoutput>LEN2:#ArrayLen(x)#|</cfoutput>
<cfcatch><cfoutput>LEN2_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset x = q2["a"]>
  <cfset ArrayAppend(x, "W")>
  <cfoutput>APP_OK</cfoutput>
<cfcatch><cfoutput>APP_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset x = q2["a"]>
  <cfoutput>ISARR:#IsArray(x)#|#IsArray(q2["a"])#|#IsArray(q2.a)#|</cfoutput>
<cfcatch><cfoutput>ISARR_ERR:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- Reads from a stored bracket column are live: a later query write shows
     through the reference. --->
<cfset x = q2["a"]>
<cfset q2["a"][2] = "YY">
<cfoutput>LIVE:#x[1]#|#x[2]#|#x[3]#|#q2.a[2]#|</cfoutput>

<!--- Writing past the last row is invisible: reads back empty and
     recordcount is unchanged. --->
<cftry>
  <cfset x = q2["a"]>
  <cfset x[4] = "w">
  <cfoutput>PHAN:#q2.recordcount#|#x[4]#|#q2.a[4]#|</cfoutput>
<cfcatch><cfoutput>PHAN_ERR:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- A degraded copy stringifies as the scalar first cell and indexes like a
     string (character access, out-of-range throws). --->
<cftry>
  <cfset x = q2["a"]>
  <cfset y = x>
  <cfoutput>SCAL:#y#|#Len(y)#|#y[1]#|</cfoutput>
<cfcatch><cfoutput>SCAL_ERR:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset x = q2["a"]>
  <cfset y = x>
  <cfoutput>SCAL2:#y[3]#|</cfoutput>
<cfcatch><cfoutput>SCAL2_ERR:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- The len() member method on a stored reference behaves like Len (first
     cell length), matching CF. --->
<cftry>
  <cfset x = q2["a"]>
  <cfoutput>MLEN:#x.len()#|</cfoutput>
<cfcatch><cfoutput>MLEN_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- An integer-column reference degrades to an Integer scalar
     (java.lang.Integer in the messages). --->
<cfset q3 = QueryNew("n", "integer", [{n:1},{n:2},{n:3}])>
<cftry>
  <cfset x = q3["n"]>
  <cfset y = x>
  <cfset y[1] = 9>
  <cfoutput>INT_OK</cfoutput>
<cfcatch><cfoutput>INT_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset x = q3["n"]>
  <cfset y = x>
  <cfoutput>INT_SCAL:#y#|</cfoutput>
<cfcatch><cfoutput>INT_SCAL_ERR:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset x = q3["n"]>
  <cfset y = x>
  <cfoutput>INT_B1:#y[1]#|</cfoutput>
<cfcatch><cfoutput>INT_B1_ERR:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- Same semantics from cfscript (second copy, live read, IsArray). --->
<cfscript>
qs2 = QueryNew("a", "varchar", [{a:"x"},{a:"y"},{a:"z"}]);
c2 = qs2["a"];
c3 = c2;
try {
    c3[1] = "MUT";
    writeOutput("SCR2_OK");
} catch (any e) {
    writeOutput("SCR2_ERR:" & e.type & "|" & e.message & "|");
}
live = qs2["a"];
qs2["a"][3] = "L3";
writeOutput("SCR2_LIVE:" & live[1] & "|" & live[2] & "|" & live[3] & "|");
writeOutput("SCR2_ISARR:" & IsArray(qs2["a"]) & "|" & IsArray(live) & "|");
try {
    ArrayLen(live);
    writeOutput("SCR2_LEN_OK");
} catch (any e) {
    writeOutput("SCR2_LEN_ERR:" & e.type & "|" & e.message & "|");
}
</cfscript>



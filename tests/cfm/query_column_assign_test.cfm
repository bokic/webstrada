<cfset q = QueryNew("a,b", "varchar,integer", [{a:"x",b:1},{a:"y",b:2},{a:"z",b:3}])>

<!--- Whole-column assignment q.a = v writes the current row's cell (row 1). --->
<cfset q.a = "W">
<cfset q.b = 42>
<cfoutput>S:#q.a[1]#|#q.a[2]#|#q.a[3]#|#q.b[1]#|#q.b[2]#|#q.b[3]#|</cfoutput>

<!--- A non-column name throws CF's columnMap Application error. --->
<cftry>
  <cfset q.zzz = "v">
  <cfoutput>N1_OK</cfoutput>
<cfcatch><cfoutput>N1:#cfcatch.type#:#cfcatch.message#:#cfcatch.detail#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset q.recordcount = 99>
  <cfoutput>N2_OK</cfoutput>
<cfcatch><cfoutput>N2:#cfcatch.type#:#cfcatch.message#:#cfcatch.detail#|</cfoutput></cfcatch>
</cftry>

<!--- Bracket whole-column assignment is rejected by CF. --->
<cftry>
  <cfset q["a"] = "W">
  <cfoutput>B1_OK</cfoutput>
<cfcatch><cfoutput>B1:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>
<cftry>
  <cfset q["zzz"] = 9>
  <cfoutput>B2_OK</cfoutput>
<cfcatch><cfoutput>B2:#cfcatch.type#:#cfcatch.message#|</cfoutput></cfcatch>
</cftry>

<!--- Same semantics from cfscript. --->
<cfscript>
qs = QueryNew("a,b", "varchar,integer", [{a:"p",b:10},{a:"q",b:20}]);
qs.a = "SP";
qs.b = 77;
writeOutput("SCR_S:" & qs.a[1] & "|" & qs.a[2] & "|" & qs.b[1] & "|" & qs.b[2] & "|");
try { qs.zzz = "v"; writeOutput("SCR_N_OK"); }
catch (any e) { writeOutput("SCR_N:" & e.type & "|" & e.message & "|" & e.detail & "|"); }
try { qs["a"] = "W"; writeOutput("SCR_B_OK"); }
catch (any e) { writeOutput("SCR_B:" & e.type & "|" & e.message & "|"); }
</cfscript>

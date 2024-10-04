<cfset br = chr(13) & chr(10)>
<cfscript>
writeOutput("before: " & IsUserLoggedIn() & "|" & GetAuthUser() & "|" & GetUserRoles() & br);
</cfscript>
<cflogin idletimeout="3600">
  <cfoutput>inside cflogin body<br></cfoutput>
  <cfloginuser name="bob" password="secret" roles="admin,user">
</cflogin>
<cfscript>
writeOutput("after: " & IsUserLoggedIn() & "|" & GetAuthUser() & "|" & GetUserRoles() & br);
writeOutput("inrole admin: " & IsUserInRole("admin") & br);
writeOutput("inrole user: " & IsUserInRole("user") & br);
writeOutput("inrole bogus: " & IsUserInRole("bogus") & br);
writeOutput("inany admin,bogus: " & IsUserInAnyRole("admin,bogus") & br);
writeOutput("inany bogus,none: " & IsUserInAnyRole("bogus,none") & br);
</cfscript>

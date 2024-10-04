<cfset br = chr(13) & chr(10)>
<cflogin>
  <cfloginuser name="gary" password="pw" roles="role one,role two,role three">
</cflogin>
<cfscript>
writeOutput("roles: " & GetUserRoles() & br);
writeOutput("inrole single: " & IsUserInRole("role one") & br);
writeOutput("inrole two: " & IsUserInRole("role one,role two") & br);
writeOutput("inrole missing: " & IsUserInRole("role two,role four") & br);
writeOutput("inany one: " & IsUserInAnyRole("role three") & br);
writeOutput("inany mixed: " & IsUserInAnyRole("x,role two") & br);
writeOutput("inany none: " & IsUserInAnyRole("x,y") & br);
</cfscript>

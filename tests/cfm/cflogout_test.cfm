<cfset br = chr(13) & chr(10)>
<cflogin>
  <cfset bodyran = "yes">
  <cfloginuser name="carol" password="pw" roles="r1,r2">
</cflogin>
<cfscript>
writeOutput("in: " & IsUserLoggedIn() & "|" & GetAuthUser() & "|" & GetUserRoles() & br);
</cfscript>
<cflogout>
<cfscript>
writeOutput("out-current: " & IsUserLoggedIn() & "|" & GetAuthUser() & "|" & GetUserRoles() & br);
</cfscript>
<cflogout session="all">
<cfscript>
writeOutput("out-all: " & IsUserLoggedIn() & br);
</cfscript>
<cflogout session="others">
<cfscript>
writeOutput("out-others: " & IsUserLoggedIn() & br);
</cfscript>

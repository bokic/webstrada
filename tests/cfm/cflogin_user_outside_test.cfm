<cfset br = chr(13) & chr(10)>
<cfloginuser name="jack" password="pw" roles="solo">
<cfscript>
writeOutput("logged: " & IsUserLoggedIn() & "|" & GetAuthUser() & "|" & GetUserRoles() & br);
</cfscript>
<cflogout>
<cfscript>
writeOutput("after-logout: " & IsUserLoggedIn() & "|" & GetAuthUser() & "|" & GetUserRoles() & br);
</cfscript>

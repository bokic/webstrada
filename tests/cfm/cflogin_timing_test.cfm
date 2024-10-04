<cfset br = chr(13) & chr(10)>
<cflogin>
  <cfset inside1 = "auth=" & GetAuthUser() & ";logged=" & IsUserLoggedIn()>
  <cfloginuser name="hank" password="pw" roles="q">
  <cfset inside2 = "auth=" & GetAuthUser() & ";logged=" & IsUserLoggedIn()>
</cflogin>
<cfscript>
writeOutput(inside1 & "|" & inside2 & "|after=" & GetAuthUser() & "|logged=" & IsUserLoggedIn() & br);
</cfscript>

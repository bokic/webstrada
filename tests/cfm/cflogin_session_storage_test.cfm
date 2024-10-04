<cfapplication name="sessapp" sessionmanagement="true" loginstorage="session">
<cfset br = chr(13) & chr(10)>
<cflogin>
  <cfloginuser name="dave" password="pw" roles="a,b">
</cflogin>
<cfscript>
writeOutput("logged: " & IsUserLoggedIn() & "|" & GetAuthUser() & "|" & GetUserRoles() & br);
writeOutput("session key def: " & isDefined("session.CFAUTHORIZATION_sessapp") & br);
if (isDefined("session.CFAUTHORIZATION_sessapp")) {
    key = session.CFAUTHORIZATION_sessapp;
    writeOutput("isbase64: " & IsBinary(BinaryDecode(key, "base64")) & br);
    dec = toString(BinaryDecode(key, "base64"), "utf-8");
    parts = listToArray(dec, chr(13));
    writeOutput("parts: " & arrayLen(parts) & "|" & parts[1] & "|" & parts[2] & "|" & len(parts[3]) & "|" & len(parts[4]) & br);
}
</cfscript>

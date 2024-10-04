<cfapplication name="csrfapp" sessionmanagement="true">
<cfscript>
t1 = CSRFGenerateToken();
t2 = CSRFGenerateToken();
writeOutput("1:[" & Len(t1) & "]");
writeOutput("2:[" & (t1 EQ t2) & "]");
t3 = CSRFGenerateToken("", true);
writeOutput("3:[" & (t1 EQ t3) & "]");
tk = CSRFGenerateToken("mykey");
writeOutput("4:[" & CSRFVerifyToken(tk, "mykey") & "]");
writeOutput("5:[" & CSRFVerifyToken("wrongtoken", "mykey") & "]");
writeOutput("6:[" & CSRFVerifyToken(t1) & "]");
</cfscript>

<cftry>
<cfscript>
function f(a="DA", b="DB", c="DC") { return "a=" & a & "|b=" & b & "|c=" & c; }
r1 = f(c="z");
r2 = f(b="y", a="x");
r3 = f("p", "q", "r");
writeOutput("r1=" & r1);
writeOutput("|r2=" & r2);
writeOutput("|r3=" & r3);
</cfscript>
<cfcatch type="any"><cfoutput>ERR:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
<cfscript>
function g(a="D", b="E") { return a & b; }
</cfscript>
<cfoutput>
|#g(b="B")#|#g(a="A")#
</cfoutput>

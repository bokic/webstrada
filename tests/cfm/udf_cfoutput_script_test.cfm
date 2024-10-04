<cfoutput><cfscript>function f() { return "x"; } writeOutput(f());</cfscript></cfoutput>
<cfoutput><cfscript>function g() { return 42; }</cfscript></cfoutput>
<cfscript>writeOutput("|" & g());</cfscript>

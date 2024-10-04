<!--- Named arguments literally named `var` / `name`: the textparser tokenizes
     var as a Keyword, and `name` used to be misparsed as a bare variable (was
     BUGS.md "var cannot be used as a named function argument" and "A script
     named argument whose argument name is name is misparsed"). --->
<cfscript>
function two(var="D", other="O") { return arguments.var & "|" & arguments.other; }
writeOutput(two(var="hello"));
writeOutput("|" & two(var="a", other="b"));
writeOutput("|" & two());
writeOutput("|" & two("pos"));
</cfscript>
|CALLBACK:
<cfscript>
c = {two: function(name="", value="") { return arguments.name & "=" & arguments.value; }};
writeOutput(c.two(name="x", value="2"));
</cfscript>
|MEMBER:
<cfscript>
function three(var="hi") { return arguments.var; }
writeOutput(three());
</cfscript>

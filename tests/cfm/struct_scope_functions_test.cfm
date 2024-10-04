<cfscript>
function addOne(x) { return x + 1; }
x = 5;

writeOutput("keyExists(variables,addOne): " & structKeyExists(variables, "addOne") & "<br>");
writeOutput("keyExists(variables,x): " & structKeyExists(variables, "x") & "<br>");
writeOutput("keyExists(variables,missing): " & structKeyExists(variables, "missing") & "<br>");
writeOutput("count(variables): " & structCount(variables) & "<br>");
writeOutput("isEmpty(variables): " & structIsEmpty(variables) & "<br>");

s = variables;
writeOutput("count(s): " & structCount(s) & "<br>");
writeOutput("keyExists(s,addOne): " & structKeyExists(s, "addOne") & "<br>");

t = {};
t.z = 1;
writeOutput("count(t): " & structCount(t) & "<br>");
writeOutput("find(t,z): " & structFind(t, "z") & "<br>");
writeOutput("keyArray(t): " & arrayLen(structKeyArray(t)) & "<br>");
</cfscript>
<cfoutput>addOne(41)=#addOne(41)#</cfoutput>

<!--- UDF variables.foo writes the page scope (was BUGS.md "UDF: variables.foo") --->
1:<cfscript>
function setPageVar() { variables.foo = 1; }
setPageVar();
writeOutput("1:[foo=" & isDefined("foo") & "|vfoo=" & variables.foo & "]");
</cfscript>
|2:<cfscript>
outer = 100;
function usesOuter() { localVar = 5; return outer + localVar; }
writeOutput("2:[" & usesOuter() & "|" & localVar & "]");
</cfscript>
|3:<cfscript>
c = createObject("component", "components/udf_vars_scope");
c.setV("X");
writeOutput("3:[page=" & isDefined("foo") & "|inst=" & c.getFoo() & "]");
</cfscript>

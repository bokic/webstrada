<cfscript>
c = new components.meta();
m = GetMetaData(c);
writeOutput("1:[" & IsStruct(m) & "]");
writeOutput("2:[" & m.TYPE & "]");
writeOutput("3:[" & m.FULLNAME & "]");
writeOutput("4:[" & m.PROPERTIES[1].NAME & "]");
writeOutput("5:[" & m.PROPERTIES[1].TYPE & "]");
writeOutput("6:[" & m.FUNCTIONS[1].NAME & "]");
writeOutput("7:[" & StructKeyExists(m, "PATH") & "]");
writeOutput("8:[" & StructKeyExists(m, "EXTENDS") & "]");
writeOutput("9:[" & StructKeyExists(m, "PROPERTIES") & "]");
function myFunc(a, b=2) { return a+b; }
u = GetMetaData(myFunc);
writeOutput("10:[" & u.NAME & "]");
writeOutput("11:[" & u.PARAMETERS[2].default & "]");
writeOutput("12:[" & u.PARAMETERS[1].required & "]");
</cfscript>

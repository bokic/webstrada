<cfscript>
// invoke() tests: UDF in current page, component instance, component path,
// positional array args, named struct args.
function myUDF(x) { return "UDF:" & x; }

r0 = invoke("", "myUDF", ["zzz"]);
writeOutput("A:[" & r0 & "]");

obj = new components.named_calc();
r1 = invoke(obj, "add", [2, 3]);
writeOutput("B:[" & r1 & "]");
r2 = invoke(obj, "add", {a:10, b:20});
writeOutput("C:[" & r2 & "]");
r3 = invoke(obj, "greet", ["bob"]);
writeOutput("D:[" & r3 & "]");
r4 = invoke(obj, "greet", {who:"carol"});
writeOutput("E:[" & r4 & "]");

// component path string, no args
p = new components.person();
r5 = invoke(p, "getSpecies", []);
writeOutput("F:[" & r5 & "]");
</cfscript>

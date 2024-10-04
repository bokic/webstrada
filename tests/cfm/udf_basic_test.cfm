<cfscript>
// UDF basic: registration, calling, hoisting, default params, arguments.
function add(a, b) {
    return a + b;
}
writeoutput("add: " & add(2, 3) & "<br>");

function greet(name, punct = "!") {
    return "Hi " & name & punct;
}
writeoutput("greet: " & greet("A") & "|" & greet("B", "?") & "<br>");

function sumAll() {
    t = 0;
    for (i = 1; i <= arrayLen(arguments); i++) {
        t = t + arguments[i];
    }
    return t;
}
writeoutput("sumAll: " & sumAll(1, 2, 3, 4) & "<br>");

function fact(n) {
    if (n <= 1) return 1;
    return n * fact(n - 1);
}
writeoutput("fact: " & fact(5) & "<br>");

function probe(a, b) {
    return structKeyList(arguments) & "|" & structCount(arguments) & "|" & arguments.a & "|" & arguments[1];
}
writeoutput("probe: " & probe("one", "two") & "<br>");
writeoutput("probe2: " & probe("one", "two", "three") & "<br>");

function noReturn() {
    x = 1 + 1;
}
writeoutput("noReturn isNull: " & isNull(noReturn()) & "<br>");

function pageFn(v) {
    return v * 2;
}
f2 = pageFn;
writeoutput("firstClass: " & f2(4) & "<br>");
writeoutput("variablesCall: " & variables.pageFn(3) & "<br>");
writeoutput("isCustom: " & IsCustomFunction(pageFn) & "<br>");
</cfscript>

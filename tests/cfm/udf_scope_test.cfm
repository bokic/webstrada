<cfscript>
// UDF scoping: var isolation, unqualified write leak, parent scope reads,
// typed params / return coercion.
function withVar() {
    var secret = 42;
    var i = 0;
    for (i = 1; i <= 3; i++) {
        var j = i * 10;
    }
    return j;
}
writeoutput("withVar: " & withVar() & "<br>");

outer = 100;
function usesOuter() {
    localVar = 5;
    return outer + localVar;
}
writeoutput("usesOuter: " & usesOuter() & "<br>");
writeoutput("leaked localVar: " & localVar & "<br>");

x = 100;
function shadowTest() {
    var x = 1;
    return x;
}
writeoutput("shadow: " & shadowTest() & "|pageX=" & x & "<br>");

y = 200;
function writeTest() {
    y = 5;
    return y;
}
writeoutput("writeTest: " & writeTest() & "|pageY=" & y & "<br>");

z = 300;
function readPage() {
    return z;
}
writeoutput("readPage: " & readPage() & "<br>");

function sq(numeric n) {
    return n * n;
}
writeoutput("typed: " & sq(5) & "|" & sq("5") & "|" & sq(true) & "<br>");

function strFn() returntype="string" {
    return 42;
}
writeoutput("ret string: [" & strFn() & "]<br>");

function numFn() returntype="numeric" {
    return "7";
}
writeoutput("ret num: " & numFn() & "<br>");

function defaulted(a, b = 10, c = "x") {
    return a & ":" & b & ":" & c;
}
writeoutput("defaults: " & defaulted(1) & "|" & defaulted(1, 2, 3) & "<br>");
</cfscript>

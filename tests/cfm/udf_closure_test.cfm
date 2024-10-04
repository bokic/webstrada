<cfscript>
// UDF closures: expressions, capture, callbacks, first-class values.
arr = [1, 2, 3, 4];
mapped = arrayMap(arr, function(x) { return x * 2; });
writeoutput("map: " & arrayToList(mapped) & "<br>");

base = 100;
adder = function(x) { return x + base; };
writeoutput("capture page var: " & adder(1) & "<br>");

function makeCounter() {
    var count = 0;
    var inc = function() {
        count = count + 1;
        return count;
    };
    return inc;
}
c = makeCounter();
writeoutput("counter: " & c() & "," & c() & "," & c() & "<br>");

function makeFactory(b) {
    return function(x) { return x * b; };
}
f5 = makeFactory(5);
f7 = makeFactory(7);
writeoutput("factory: " & f5(3) & "," & f7(3) & "<br>");

s = {};
s.fn = function(x) { return x + 10; };
writeoutput("member: " & s.fn(1) & "<br>");

function invokeIt(fnRef, v) {
    return fnRef(v);
}
writeoutput("pass fn: " & invokeIt(adder, 9) & "<br>");

anon = function() { return "NOARGS"; };
writeoutput("noargs: " & anon() & "|" & anon(1, 2) & "<br>");

writeoutput("isClosure: " & IsClosure(anon) & "|" & IsClosure(add) & "<br>");
writeoutput("isCustom: " & IsCustomFunction(anon) & "|" & IsCustomFunction(add) & "<br>");
function add(a, b) {
    return a + b;
}
</cfscript>

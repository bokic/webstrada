<!--- The `arguments` scope and function-local parameters share the same slot:
writing an unqualified param name inside a function updates `arguments.x`, and
writing `arguments.x` updates the param — CF keeps the same Variable object in
both scopes. Verified against CF 2025. --->
<cfscript>
function f(a) {
  a = "mutated";
  return "f:" & arguments.a;
}
function g(b) {
  arguments.b = "viaargs";
  return "g:" & b;
}
function h(c) {
  var d = "dorig";
  return "h:" & c & "|" & d;
}
writeOutput(f("orig") & "|" & g("orig") & "|" & h("carg"));
</cfscript>

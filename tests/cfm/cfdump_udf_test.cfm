<cfscript>
function addOne(x) { return x + 1; }
function addTwo(numeric a, string b="hello", numeric c=10) { return a + c; }
function greet(name) { return "hi " & name; }
</cfscript>
<cfoutput>#addOne(41)#|#addTwo(1)#|#greet("x")#</cfoutput>
<cfdump var="#variables#">
<cfset fn = variables.addOne>
<cfdump var="#fn#">
<cfset st = {single: variables.addOne, greet: variables.greet}>
<cfdump var="#st#">
<cfset arr = [variables.addTwo, 42]>
<cfdump var="#arr#">
<cfset nested = {inner: {fn: variables.greet}}>
<cfdump var="#nested#">
<cfdump var="#st#" format="text">

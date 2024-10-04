<!--- CallStackGet: array of {Template, LineNumber, Function} frames --->
<cfscript>
function level3() {
    return CallStackGet();
}
function level2() {
    return level3();
}
function level1() {
    return level2();
}
</cfscript>
|nested:
<cfset cs = level1()>
<cfoutput>#arrayLen(cs)#</cfoutput>
<cfloop index="i" from="1" to="#arrayLen(cs)#">
<cfoutput>[#i#]#cs[i].function#:#cs[i].lineNumber#</cfoutput>
</cfloop>
|top-level:
<cfset cs2 = CallStackGet()>
<cfoutput>#arrayLen(cs2)#:#cs2[1].function#:#cs2[1].lineNumber#</cfoutput>

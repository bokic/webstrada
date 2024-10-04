P<cfloop index="wi" from="1" to="1">L</cfloop>Q
P<cfloop index="wi" from="1" to="2">L</cfloop>Q
P<cfloop index="wi" from="1" to="2">A<cfset wi2 = wi>B</cfloop>Q
P<cfloop index="wi" from="1" to="1">A<cfset wi2 = 1></cfloop>Q
P<cfloop index="wi" from="1" to="1"><cfset wi2 = 1>B</cfloop>Q
P<cfloop index="wi" from="1" to="2">A<cfset wi2 = wi>B<cfoutput>#wi#</cfoutput>C</cfloop>Q
P<cfloop index="wi" from="1" to="2">
A
</cfloop>Q
P<cfloop index="wi" from="1" to="2">X
Y</cfloop>Q
P<cfloop index="wi" from="1" to="2">x<cfoutput>#wi#</cfoutput>y</cfloop>Q
P<cfloop index="wi" from="1" to="2"></cfloop>Q
P<cfloop index="wi" from="3" to="2">L</cfloop>Q
P<cfloop index="wi" from="1" to="2">A<cfif true>Y</cfif>B</cfloop>Q
P<cfloop index="wi" from="1" to="2">A<cfloop index="wi2" from="1" to="2">B</cfloop>C</cfloop>Q
P<cfloop index="wi" from="1" to="2">A<b>B</b>C</cfloop>Q
<cfoutput>P<cfloop index="wi" from="1" to="2">L</cfloop>Q</cfoutput>
P<cfloop index="wi" from="1" to="2">
</cfloop>Q

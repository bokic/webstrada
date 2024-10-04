P<cfloop index="i" from="1" to="1"><cfset q = 5>#q#</cfloop>Q
P<cfloop index="i" from="1" to="2">A#q#B</cfloop>Q
P<cfloop index="i" from="1" to="2">#q#</cfloop>Q
P<cfloop index="i" from="1" to="2"><cfif true>#q#</cfif></cfloop>Q
P<cfloop from="1" to="2" index="i"><cfloop from="1" to="1" index="j">#q#</cfloop></cfloop>Q
P<cfloop index="i" from="1" to="2">##</cfloop>Q
<cfoutput>P<cfloop index="i" from="1" to="2">#i#</cfloop>Q</cfoutput>
<cfoutput>P<cfloop index="i" from="1" to="2">A#i#B</cfloop>Q</cfoutput>
<cfset x = 9><cfoutput>P<cfif true>IF#x#END</cfif>Q</cfoutput>
<cfset x = 5><cfoutput>P<cfloop index="i" from="1" to="2">#i#=#x#;</cfloop>Q</cfoutput>
<cfoutput>P<cfloop from="1" to="2" index="i"><cfloop from="1" to="2" index="j">#i#:#j#;</cfloop></cfloop>Q</cfoutput>
<cfoutput>P<cfloop index="i" from="1" to="2">#i#:##;</cfloop>Q</cfoutput>

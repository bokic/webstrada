<!--- cfloop numeric bounds: multi-interpolation (was BUGS.md #26) --->
<cfset x = 5>
<cfset y = 7>
A:<cfloop from="1" to="#x##y#" index="i"><cfoutput>#i#</cfoutput></cfloop>
|B:<cfloop from="1" to="#x#5" index="i"><cfoutput>#i#</cfoutput></cfloop>
|C:<cfloop from="5#x#" to="7" index="i"><cfoutput>#i#</cfoutput></cfloop>
|D:<cfloop from="1" to="#x + y#" index="i"><cfoutput>#i#</cfoutput></cfloop>
|E:<cfloop from="1" to="#5+2#" index="i"><cfoutput>#i#</cfoutput></cfloop>
|F:<cfloop from="1.0" to="3.0" index="i"><cfoutput>#i#</cfoutput></cfloop>
|G:<cfloop from=" 1 " to=" 3 " index="i"><cfoutput>#i#</cfoutput></cfloop>

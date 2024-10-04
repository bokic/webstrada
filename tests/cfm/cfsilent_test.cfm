<cfoutput>START|</cfoutput>
<cfsilent>
<cfset x = 42>
<cfoutput>HIDDEN_#x#</cfoutput>
<cfscript>writeOutput("SCRIPT_HIDDEN");</cfscript>
</cfsilent>
<cfoutput>|X=#x#</cfoutput>

<cfoutput>NEST_OUTER|</cfoutput>
<cfsilent>
<cfset y = 7>
<cfsilent>
<cfoutput>NESTED_HIDDEN</cfoutput>
</cfsilent>
<cfoutput>OUTER_HIDDEN</cfoutput>
</cfsilent>
<cfoutput>|Y=#y#</cfoutput>

<cfoutput>LOOP|</cfoutput>
<cfloop from="1" to="3" index="i">
<cfsilent>
<cfoutput>#i#</cfoutput>
</cfsilent>
<cfoutput>#i#</cfoutput>
</cfloop>
<cfoutput>|LOOPEND</cfoutput>

<cfoutput>SELFCLOSE|</cfoutput>
<cfsilent/>
<cfoutput>|AFTERSELF</cfoutput>

<cfset arr = [1, 2, 3]>
<cfoutput>DUMP|</cfoutput>
<cfsilent>
<cfdump var="#arr#">
</cfsilent>
<cfoutput>|DUMPEND</cfoutput>

<cfsilent>
plain text body is suppressed
<cfset z = "plain">
</cfsilent>
<cfoutput>|PLAINTEXT</cfoutput>

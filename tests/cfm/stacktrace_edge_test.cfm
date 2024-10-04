<!--- stacktrace: errors in cfoutput expressions and inside loops --->
|1: division by zero inside a #...# cfoutput expression:
<cftry>
    <cfoutput>#1 / 0#</cfoutput>
    <cfcatch type="any">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>
|2: error inside a cfloop body:
<cftry>
    <cfloop index="i" from="1" to="3">
        <cfset loopVar = 1 / (i - 2)>
    </cfloop>
    <cfcatch type="any">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>
|3: error caught by the innermost try:
<cftry>
    <cftry>
        <cfset innerErr2 = "a" + 5>
        <cfcatch type="any">
            <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
            <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
                <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
            </cfloop>
        </cfcatch>
    </cftry>
    <cfcatch type="any">
        <cfoutput>WRONG</cfoutput>
    </cfcatch>
</cftry>
|4: after a catch the stack is clean again:
<cftry>
    <cfset cleaned = 1 / 0>
    <cfcatch type="any">
        <cfoutput>caught</cfoutput>
    </cfcatch>
</cftry>
<cftry>
    <cfset cleaned2 = 7 * 6>
    <cfcatch type="any">
        <cfoutput>BAD</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>done</cfoutput>

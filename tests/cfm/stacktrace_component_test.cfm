<!--- stacktrace: component method chain frames --->
<cfset comp = CreateObject("component", "components/stacktrace_comp")>
|1: tag-form method -> method -> throw:
<cftry>
    <cfset comp.outerCall()>
    <cfcatch type="compErr">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>
|2: script-form method throw:
<cftry>
    <cfset comp.scriptThrow()>
    <cfcatch type="compErr2">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>

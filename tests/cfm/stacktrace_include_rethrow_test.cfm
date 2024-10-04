<!--- stacktrace: cfinclude chain frames + rethrow preservation --->
<cfscript>
function rethrowOuter() {
    try {
        throw(type = "rtErr", message = "rethrow me");
    } catch (rtErr e) {
        rethrow;
    }
}
</cfscript>
|1: error inside an included page:
<cftry>
    <cfinclude template="/include_lib/stacktrace_inc.cfm">
    <cfcatch type="incErr">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>
|2: rethrow keeps the throw-site stack:
<cftry>
    <cfset rethrowOuter()>
    <cfcatch type="rtErr">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>

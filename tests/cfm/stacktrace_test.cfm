<!--- stacktrace: call-stack tracking for error reporting (tagContext lines) --->
<!--- line 2: header comment --->
<cfscript>
function innerThrow() {
    throw(type = "myErr", message = "boom");
}
function outerCall() {
    return innerThrow();
}
function divZero() {
    return 1 / 0;
}
</cfscript>
|1: nested UDF throw:
<cftry>
    <cfset outerCall()>
    <cfcatch type="myErr">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>
|2: div-by-zero in UDF:
<cftry>
    <cfset divZero()>
    <cfcatch type="any">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>
|3: tag-form error line:
<cftry>
    <cfset y = 1 / 0>
    <cfcatch type="any">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>
|4: script error line:
<cftry>
    <cfscript>
        z = "abc" + 1;
    </cfscript>
    <cfcatch type="any">
        <cfoutput>len=#arrayLen(cfcatch.tagContext)#</cfoutput>
        <cfloop index="i" from="1" to="#arrayLen(cfcatch.tagContext)#">
            <cfoutput>[#i#]:#cfcatch.tagContext[i].line#</cfoutput>
        </cfloop>
    </cfcatch>
</cftry>

<cftry>
    <cfset x = 1 / 0>
    <cfoutput>1UNREACHABLE</cfoutput>
    <cfcatch type="any">
        <cfoutput>1:#cfcatch.type#:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>|</cfoutput>

<cftry>
    <cfthrow type="myType" message="boom" detail="d" errorcode="7" extendedinfo="ei">
    <cfcatch type="myType">
        <cfoutput>2:#cfcatch.type#:#cfcatch.message#:#cfcatch.detail#:#cfcatch.errorcode#:#cfcatch.extendedinfo#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>|</cfoutput>

<cftry>
    <cfthrow message="nomsg">
    <cfcatch type="t2">
        <cfoutput>3WRONG</cfoutput>
    </cfcatch>
    <cfcatch type="application">
        <cfoutput>3:#cfcatch.type#:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>|</cfoutput>

<cftry>
    <cfset m = "dyn">
    <cfthrow type="t1" message="#m#">
    <cfcatch type="t1">
        <cfoutput>4:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>|</cfoutput>

<cftry>
    <cfoutput>5ok</cfoutput>
    <cffinally>
        <cfoutput>5fin</cfoutput>
    </cffinally>
</cftry>
<cfoutput>|</cfoutput>

<cftry>
    <cfoutput>6t</cfoutput>
    <cfcatch>
        <cfoutput>6UNREACHABLE</cfoutput>
    </cfcatch>
    <cffinally>
        <cfoutput>6f</cfoutput>
    </cffinally>
</cftry>
<cfoutput>|</cfoutput>

<cftry>
    <cftry>
        <cfthrow message="original">
        <cfcatch type="any">
            <cfoutput>7inner:#cfcatch.message#;</cfoutput>
            <cfrethrow>
        </cfcatch>
    </cftry>
    <cfcatch type="any">
        <cfoutput>7outer:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>|</cfoutput>

<cftry>
    <cftry>
        <cfthrow message="boom">
        <cfcatch type="any">
            <cfoutput>8caught:#cfcatch.message#;</cfoutput>
        </cfcatch>
        <cffinally>
            <cfoutput>8fin;</cfoutput>
        </cffinally>
    </cftry>
    <cfcatch type="any">
        <cfoutput>8outer:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>|</cfoutput>

<cfloop from="1" to="3" index="i">
    <cftry>
        <cfif i eq 2>
            <cfthrow message="at#i#">
        </cfif>
        <cfoutput>9:#i#;</cfoutput>
        <cfcatch type="any">
            <cfoutput>9caught:#cfcatch.message#;</cfoutput>
        </cfcatch>
    </cftry>
</cfloop>
<cfoutput>|</cfoutput>

<cftry>
    <cftry>
        <cfthrow message="original">
        <cfcatch type="any">
            <cfoutput>10caught;</cfoutput>
        </cfcatch>
        <cffinally>
            <cfset z = 1 / 0>
        </cffinally>
    </cftry>
    <cfcatch type="any">
        <cfoutput>10outer:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>done</cfoutput>

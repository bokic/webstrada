<cffunction name="boolProbe" returntype="string">
    <cfargument name="active" type="boolean" required="true" />
    <cfreturn "ok" />
</cffunction>

<cftry>
    <cfoutput>#boolProbe("administrator")#</cfoutput>
    <cfcatch type="any">
        <cfoutput>CAUGHT:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>

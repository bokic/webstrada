<cfset result = "">
<cfloop array="#['a', 'b']#" item="item">
    <cfset result = result & item>
</cfloop>
<cfoutput>#result#</cfoutput>

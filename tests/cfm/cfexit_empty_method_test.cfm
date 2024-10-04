<!--- <cfexit method=""> renders the value as '' in the runtime validation
     message (was BUGS.md "<cfexit method=""> renders '' on CF"). Static and
     dynamically evaluated empty/invalid methods. --->
<cfset x = "">
<cftry><cfexit method="#x#">X<cfcatch type="any"><cfoutput>#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
|DYN-BOGUS:
<cfset y = "bogus">
<cftry><cfexit method="#y#">X<cfcatch type="any"><cfoutput>#cfcatch.detail#</cfoutput></cfcatch></cftry>
|DYNAMIC-VALID:
<cfset z = "exittag">
<cftry><cfexit method="#z#">X<cfcatch type="any"><cfoutput>CAUGHT</cfoutput></cfcatch></cftry>
AFTER

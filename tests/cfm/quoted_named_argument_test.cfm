<cffunction name="namedProbe" returntype="string">
    <cfargument name="emailto" required="false" default="" />
    <cfargument name="subject" required="false" default="" />
    <cfreturn arguments.emailto & ":" & arguments.subject />
</cffunction>

<cfoutput>#namedProbe('emailto' = 'a@example.com', 'subject' = 'hello')#</cfoutput>

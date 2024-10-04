<cfoutput>START;</cfoutput>
<cfset p = CreateObject("component", "components/person")>
<cfset p.init("Sam")>
<cfoutput>#p.greet()#|</cfoutput>
<cfset q = new components.person("Page")>
<cfset pageVar = "SHOULD_NOT_LEAK">
<cftry>
<cfoutput>#q.getName()#</cfoutput>
<cfcatch><cfoutput>E:#cfcatch.message#</cfoutput></cfcatch>
</cftry>
<cfoutput>|#p.echoThis().getName()#|</cfoutput>

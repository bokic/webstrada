<cfset obj = createObject("component", "org.mangoblog.Greeter")>
<cfoutput>#obj.greet()#|</cfoutput>
<cfinclude template="/org/mangoblog/inc.cfm">

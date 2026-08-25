<cfset c = createObject("component", "components.qoq_component")>
<cfoutput>#c.getCategories(false)#|#c.getCategories(true)#</cfoutput>

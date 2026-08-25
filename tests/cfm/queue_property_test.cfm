<cfset q = createObject("component", "components.queue_property")>
<cfset q.addElement("x", 5)>
<cfoutput>#arrayLen(q.getElements())#</cfoutput>

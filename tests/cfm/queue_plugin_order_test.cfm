<cfset q = createObject("component", "components.queue_plugin_order")>
<cfset q.add("first", 5)>
<cfset q.add("second", 100)>
<cfoutput>#arrayLen(q.read())#:#q.read()[1].obj#:#q.read()[2].obj#</cfoutput>

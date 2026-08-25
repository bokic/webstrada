<cfset q = createObject("component", "components.queue_plugin_wrapper")>
<cfset q.addListener("afterCommentAdd")>
<cfoutput>#arrayLen(q.read("afterCommentAdd"))#</cfoutput>

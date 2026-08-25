<cfsetting enablecfoutputonly="true">
<cfif thisTag.executionMode EQ "start">
	<cfset currentItem = "item-1" />
	<cfoutput>[COLLIDE-START]</cfoutput>
</cfif>
<cfif thisTag.executionMode EQ "end">
	<cfoutput>[COLLIDE-END]</cfoutput>
</cfif>
<cfsetting enablecfoutputonly="false">

<cfsetting enablecfoutputonly="true">
<cfif thisTag.executionmode is 'start'>
	<cfset data = GetBaseTagData("cf_collide",1)/>
	<cfoutput>[PROP:#data.currentItem#]</cfoutput>
</cfif>
<cfsetting enablecfoutputonly="false">

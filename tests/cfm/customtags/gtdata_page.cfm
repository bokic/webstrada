<cfsetting enablecfoutputonly="true">
<cfif thisTag.executionmode is "start">
	<cfset ancestorlist = getbasetaglist() />
	<cfif listfindnocase(ancestorlist,"cf_gtdata_pages")>
		<cfset data = GetBaseTagData("cf_gtdata_pages")/>
		<cfoutput>[PAGE:#data.currentPage#:#data.counter#:#data.to#]</cfoutput>
	<cfelse>
		<cfoutput>[NO-BASE]</cfoutput>
	</cfif>
</cfif>
<cfsetting enablecfoutputonly="false">

<cfsetting enablecfoutputonly="true">
<cfparam name="attributes.from" type="numeric" default="1">
<cfparam name="attributes.count" type="numeric" default="3">
<cfif thisTag.executionMode EQ "start">
	<cfset to = attributes.count />
	<cfset counter = attributes.from />
	<cfset currentPage = "page-" & counter />
	<cfoutput>[PAGES-START:#attributes.from#:#to#]</cfoutput>
</cfif>
<cfif thisTag.executionMode EQ "end">
	<cfset counter = counter + 1>
	<cfif counter LTE to>
		<cfset currentPage = "page-" & counter><cfsetting enablecfoutputonly="false"><cfexit method="loop">
	</cfif>
	<cfoutput>[PAGES-END]</cfoutput>
</cfif>
<cfsetting enablecfoutputonly="false">

<cffunction name="acceptXml" returntype="boolean">
	<cfargument name="data" type="xml" required="true" />
	<cfreturn isXmlDoc(arguments.data) />
</cffunction>
<cfset x = XmlParse('<a><b>text</b></a>') />
<cfoutput>XML:#acceptXml(x)#</cfoutput>
<cfset s = structNew() />
<cftry>
	<cfset acceptXml(s) />
	<cfoutput>|STRUCT_ACCEPTED</cfoutput>
<cfcatch type="any">
	<cfoutput>|STRUCT_REJECTED</cfoutput>
</cfcatch>
</cftry>
<cfset q = queryNew("col") />
<cftry>
	<cfset acceptXml(q) />
	<cfoutput>|QUERY_ACCEPTED</cfoutput>
<cfcatch type="any">
	<cfoutput>|QUERY_REJECTED</cfoutput>
</cfcatch>
</cftry>

<cfsetting enablecfoutputonly="true">
<cfif thisTag.executionmode is "start">
	<cfoutput>#dateformat("2026-08-25 05:01:05",attributes.dateformat)#</cfoutput>
	<cfoutput>#timeformat("2026-08-25 05:01:05",attributes.timeformat)#</cfoutput>
	<cfoutput>#len("abc")#</cfoutput>
	<cfoutput>#ucase(attributes.ucase)#</cfoutput>
</cfif>
<cfsetting enablecfoutputonly="false">

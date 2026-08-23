<cfset form.prefix = "tbl_">
<cfset form.datasource = "my_dsn">
<cfparam name="prefix" default="default_prefix">
<cfparam name="datasource" default="default_dsn">
<cfparam name="extra" default="extra_val">
<cfoutput>PREFIX=#prefix#|DSN=#datasource#|EXTRA=#extra#|FORM_PREFIX=#form.prefix#</cfoutput>

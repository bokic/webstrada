<!--- stacktrace: cferror handler page reads error.rootcause.tagContext --->
<cfscript>
function boom() {
    throw(type = "boom", message = "x");
}
</cfscript>
<cferror type="exception" exception="boom" template="include_lib/stacktrace_err_page.cfm">
<cfset boom()>

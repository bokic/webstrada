<cfsetting enablecfoutputonly="true">
<cfif thisTag.executionMode EQ "start">
<cfset full = getbasetaglist() />
<cfset ancestorlist = listdeleteat(full,1) />
<cfoutput>[FULL:#full#][ANC:#ancestorlist#][FIND:#listfindnocase(ancestorlist,"cf_contextcheck")#]</cfoutput>
</cfif>
<cfsetting enablecfoutputonly="false">

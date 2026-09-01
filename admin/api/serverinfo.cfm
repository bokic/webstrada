<!---
  Admin API: dashboard runtime statistics (GET JSON).
  Backed by the __serverInfo() compiler-extension function.
  Query param ?excludeAdmin=true|false (default false): drops recent requests
  whose template path starts with /webstrada (the dashboard's admin-requests
  switch; filtering happens server-side).
--->
<cfsetting enablecfoutputonly="yes">
<cfcontent type="application/json" charset="utf-8">
<cfset reqMethod = CGI.REQUEST_METHOD>

<cfif reqMethod EQ "GET">
  <cfset excludeAdmin = false>
  <cfif StructKeyExists(URL, "excludeAdmin") AND URL.excludeAdmin EQ "true">
    <cfset excludeAdmin = true>
  </cfif>
  <cfset limit = 10>
  <cfif StructKeyExists(URL, "limit") AND IsNumeric(URL.limit)>
    <cfset limit = Val(URL.limit)>
  </cfif>
  <cfset beforeId = 0>
  <cfif StructKeyExists(URL, "beforeId") AND IsNumeric(URL.beforeId)>
    <cfset beforeId = Val(URL.beforeId)>
  </cfif>
  <cfset sinceId = 0>
  <cfif StructKeyExists(URL, "sinceId") AND IsNumeric(URL.sinceId)>
    <cfset sinceId = Val(URL.sinceId)>
  </cfif>
  <cfoutput>#SerializeJSON(__serverInfo(excludeAdmin, limit, beforeId, sinceId))#</cfoutput>
<cfelse>
  <cfheader statuscode="405">
  <cfoutput>{"ok":false,"error":"Method not supported"}</cfoutput>
</cfif>

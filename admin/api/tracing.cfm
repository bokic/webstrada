<!---
  Admin API: execution tracing (GET JSON).
  Backed by the __serverInfo() compiler-extension function.
  Query param ?excludeAdmin=true|false (default false).
--->
<cfsetting enablecfoutputonly="yes">
<cfcontent type="application/json" charset="utf-8">
<cfset reqMethod = CGI.REQUEST_METHOD>

<cfif reqMethod EQ "GET">
  <cfif StructKeyExists(URL, "requestId") AND IsNumeric(URL.requestId)>
    <cfset reqId = Val(URL.requestId)>
    <cfset excludeLine = false>
    <cfif StructKeyExists(URL, "excludeLine") AND (URL.excludeLine EQ "true" OR URL.excludeLine EQ "1")>
      <cfset excludeLine = true>
    </cfif>
    <cfoutput>#SerializeJSON(__requestTrace(reqId, excludeLine))#</cfoutput>
  <cfelse>
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
  </cfif>
<cfelse>
  <cfheader statuscode="405">
  <cfoutput>{"ok":false,"error":"Method not supported"}</cfoutput>
</cfif>

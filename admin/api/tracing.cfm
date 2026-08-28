<!---
  Admin API: execution tracing (GET JSON).
  Backed by the __serverInfo() compiler-extension function.
  Query param ?excludeAdmin=true|false (default false).
--->
<cfsetting enablecfoutputonly="yes">
<cfcontent type="application/json" charset="utf-8">
<cfset reqMethod = CGI.REQUEST_METHOD>

<cfif reqMethod EQ "GET">
  <cfset excludeAdmin = false>
  <cfif StructKeyExists(URL, "excludeAdmin") AND URL.excludeAdmin EQ "true">
    <cfset excludeAdmin = true>
  </cfif>
  <cfoutput>#SerializeJSON(__serverInfo(excludeAdmin))#</cfoutput>
<cfelse>
  <cfheader statuscode="405">
  <cfoutput>{"ok":false,"error":"Method not supported"}</cfoutput>
</cfif>

<!---
  Admin API: cache (GET/POST JSON).
  GET  -> __cacheInfo() listing + aggregate stats.
  POST -> { action: "clear" }                         -> __cacheClear()
          { action: "evict", region, id }             -> __cacheEvict(region, id)
--->
<cfsetting enablecfoutputonly="yes">
<cfcontent type="application/json" charset="utf-8">
<cfset reqMethod = CGI.REQUEST_METHOD>

<cfif reqMethod EQ "GET">
  <cfoutput>#SerializeJSON(__cacheInfo())#</cfoutput>

<cfelseif reqMethod EQ "POST">
  <cftry>
    <cfset body = GetHttpRequestData().content>
    <cfset payload = DeserializeJSON(body)>
    <cfif StructKeyExists(payload, "action")>
      <cfif payload.action EQ "clear">
        <cfoutput>#SerializeJSON(__cacheClear())#</cfoutput>
      <cfelseif payload.action EQ "evict">
        <cfoutput>#SerializeJSON(__cacheEvict(payload.region, payload.id))#</cfoutput>
      <cfelse>
        <cfheader statuscode="400">
        <cfoutput>{"ok":false,"error":"Unknown action"}</cfoutput>
      </cfif>
    <cfelse>
      <cfheader statuscode="400">
      <cfoutput>{"ok":false,"error":"Missing action"}</cfoutput>
    </cfif>
    <cfcatch type="any">
      <cfheader statuscode="400">
      <cfoutput>#SerializeJSON({ok: false, error: cfcatch.message})#</cfoutput>
    </cfcatch>
  </cftry>

<cfelse>
  <cfheader statuscode="405">
  <cfoutput>{"ok":false,"error":"Method not supported"}</cfoutput>
</cfif>

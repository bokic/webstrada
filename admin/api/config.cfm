<!---
  Admin API: server configuration (GET/POST JSON).
  GET  -> { settings, datasources }  (effective config; passwords masked)
  POST -> merge-update from the JSON body { settings?, datasources? }
  Backed by the __configGet()/__configSet() compiler-extension functions.
--->
<cfsetting enablecfoutputonly="yes">
<cfcontent type="application/json" charset="utf-8">
<cfset reqMethod = CGI.REQUEST_METHOD>

<cfif reqMethod EQ "GET">
<cfoutput>#SerializeJSON(__configGet())#</cfoutput>

<cfelseif reqMethod EQ "POST">
  <cftry>
    <cfset body = GetHttpRequestData().content>
    <cfset payload = DeserializeJSON(body)>
    <cfif StructKeyExists(payload, "action") AND payload.action EQ "reset">
      <cfoutput>#SerializeJSON(__configReset())#</cfoutput>
    <cfelse>
      <cfoutput>#SerializeJSON(__configSet(payload))#</cfoutput>
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

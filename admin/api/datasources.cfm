<!---
  Admin API: datasources (GET/POST JSON).
  GET                          -> { datasources: { name: {...} } } (passwords masked)
  POST with JSON body:
    { name, action: "verify" } -> __datasourceTest(name) result
    { name, action: "delete" } -> removes the datasource
    { name, backend?, host?, port?, database?, username?, password? }
                                 upsert (merge) the datasource
--->
<cfsetting enablecfoutputonly="yes">
<cfcontent type="application/json" charset="utf-8">
<cfset reqMethod = CGI.REQUEST_METHOD>

<cfif reqMethod EQ "GET">
  <cfset cfg = __configGet()>
  <cfoutput>#SerializeJSON(cfg.datasources)#</cfoutput>

<cfelseif reqMethod EQ "POST">
  <cftry>
    <cfset body = GetHttpRequestData().content>
    <cfset payload = DeserializeJSON(body)>
    <cfset name = payload.name>

    <cfif StructKeyExists(payload, "action")>
      <cfif payload.action EQ "verify">
        <cfoutput>#SerializeJSON(__datasourceTest(name))#</cfoutput>
      <cfelseif payload.action EQ "delete">
        <cfset wrapped = StructNew()>
        <cfset dsList = StructNew()>
        <cfset del = StructNew()>
        <cfset del.action = "delete">
        <cfset dsList[name] = del>
        <cfset wrapped.datasources = dsList>
        <cfoutput>#SerializeJSON(__configSet(wrapped))#</cfoutput>
      <cfelse>
        <cfheader statuscode="400">
        <cfoutput>{"ok":false,"error":"Unknown action '&payload.action&'"}</cfoutput>
      </cfif>
    <cfelse>
      <cfset wrapped = StructNew()>
      <cfset dsList = StructNew()>
      <cfset dsFields = StructNew()>
      <cfloop list="backend,host,port,database,username,password" index="f">
        <cfif StructKeyExists(payload, f)>
          <cfset dsFields[f] = payload[f]>
        </cfif>
      </cfloop>
      <cfset dsList[name] = dsFields>
      <cfset wrapped.datasources = dsList>
      <cfoutput>#SerializeJSON(__configSet(wrapped))#</cfoutput>
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

<!--- cfcache: client-side caching headers and the tag's catchable validations,
byte-verified against CF 2025 (the sqlite-backed cache store behaviors that
the RDS host cannot run - the "caching" package is not installed - live in
tier2_cfcache_test.cfm + the CfcacheTagTest unit tests). --->
<cfcache action="clientcache">
<cfoutput>CLIENTBODY</cfoutput>
</cfcache>
<cftry>
  <cfcache action="clientcache" protocol="ftp">
  <cfcatch>
    <cfoutput>CAUGHT:#cfcatch.type#:#cfcatch.message#</cfoutput>
  </cfcatch>
</cftry>
<cftry>
  <cfcache action="clientcache" key="k" region="r">
  <cfcatch>
    <cfoutput>|CAUGHT2:#cfcatch.type#:#cfcatch.message#</cfoutput>
  </cfcatch>
</cftry>
<cftry>
  <cfcache action="cache" metadata="md">
  <cfcatch>
    <cfoutput>|CAUGHT3:#cfcatch.type#:#cfcatch.message#</cfoutput>
  </cfcatch>
</cftry>
<cftry>
  <cfcache action="get" id="" name="n">
  <cfcatch>
    <cfoutput>|CAUGHT4:#cfcatch.type#:#cfcatch.message#</cfoutput>
  </cfcatch>
</cftry>
<cftry>
  <cfcache action="clientcache" dependson="a,,b">
  <cfcatch>
    <cfoutput>|CAUGHT5:#cfcatch.type#:#cfcatch.message#</cfoutput>
  </cfcatch>
</cftry>

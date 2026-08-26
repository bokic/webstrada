<cfcomponent>
  <cffunction name="formatAge" access="private" returntype="string" output="false">
    <cfargument name="value" required="true">
    <cfreturn "ok">
  </cffunction>

  <cffunction name="render" returntype="string" output="false">
    <cfset var direct = formatAge(1)>
    <cfset var interpolated = "#formatAge(2)#">
    <cfset var content = "">
    <cfsavecontent variable="content"><cfoutput>#formatAge(3)#|#formatAge(4) & "!"#</cfoutput></cfsavecontent>
    <cfreturn direct & "|" & interpolated & "|" & content>
  </cffunction>
</cfcomponent>

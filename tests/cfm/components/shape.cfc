<cfcomponent>
  <cffunction name="init" returntype="component" output="false">
    <cfargument name="color" type="string" required="false" default="red">
    <cfset this.color = arguments.color>
    <cfset variables.kind = "shape">
    <cfreturn this>
  </cffunction>
  <cffunction name="describe" returntype="string" output="false">
    <cfreturn "a " & this.color & " " & variables.kind>
  </cffunction>
  <cffunction name="area" returntype="numeric" output="false">
    <cfreturn 0>
  </cffunction>
</cfcomponent>

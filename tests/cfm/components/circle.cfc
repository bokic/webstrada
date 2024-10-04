<cfcomponent extends="shape">
  <cffunction name="init" returntype="component" output="false">
    <cfargument name="color" type="string" required="false" default="blue">
    <cfargument name="radius" type="numeric" required="false" default="1">
    <cfset this.color = arguments.color>
    <cfset variables.kind = "circle">
    <cfset this.radius = arguments.radius>
    <cfreturn this>
  </cffunction>
  <cffunction name="area" returntype="numeric" output="false">
    <cfreturn 3.14 * this.radius * this.radius>
  </cffunction>
</cfcomponent>

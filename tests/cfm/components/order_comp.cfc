<cfcomponent>
  <cfproperty name="age" type="numeric" default="0">
  <cfproperty name="active" type="boolean" default="true">
  <cfproperty name="score" type="numeric" default="99.5">
  <cfproperty name="title" type="string" default="hello">
  <cffunction name="init" returntype="component">
    <cfset this.z = 100>
    <cfset this.a = 200>
    <cfreturn this>
  </cffunction>
  <cffunction name="foo"></cffunction>
  <cffunction name="bar"></cffunction>
</cfcomponent>

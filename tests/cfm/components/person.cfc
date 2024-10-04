<cfcomponent>
  <cfproperty name="fullname" type="string" default="NOBODY">
  <cffunction name="init" returntype="component" output="false">
    <cfargument name="name" type="string" required="false" default="John">
    <cfset this.name = arguments.name>
    <cfset variables.species = "human">
    <cfreturn this>
  </cffunction>
  <cffunction name="getName" returntype="string" output="false">
    <cfreturn this.name>
  </cffunction>
  <cffunction name="setName" returntype="void" output="false">
    <cfargument name="value" type="string" required="true">
    <cfset this.name = arguments.value>
  </cffunction>
  <cffunction name="getSpecies" returntype="string" output="false">
    <cfreturn variables.species>
  </cffunction>
  <cffunction name="greet" returntype="string" output="false">
    <cfargument name="who" type="string" required="false" default="world">
    <cfreturn "Hello " & arguments.who>
  </cffunction>
  <cffunction name="secret" returntype="string" access="private" output="false">
    <cfreturn "PRIVATE_SECRET">
  </cffunction>
  <cffunction name="reveal" returntype="string" output="false">
    <cfreturn secret()>
  </cffunction>
  <cffunction name="add" returntype="numeric" output="false">
    <cfargument name="a" type="numeric" required="true">
    <cfargument name="b" type="numeric" required="true">
    <cfreturn arguments.a + arguments.b>
  </cffunction>
  <cffunction name="echoThis" returntype="any" output="false">
    <cfreturn this>
  </cffunction>
</cfcomponent>

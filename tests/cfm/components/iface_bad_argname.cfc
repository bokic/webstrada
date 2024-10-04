<cfcomponent implements="iface_animal">
  <cffunction name="getSound" returntype="string" output="false">
    <cfargument name="vol" type="numeric" required="false" default="5">
    <cfreturn "Woof" & arguments.vol>
  </cffunction>
  <cffunction name="getLegs" returntype="numeric" output="false">
    <cfreturn 4>
  </cffunction>
</cfcomponent>

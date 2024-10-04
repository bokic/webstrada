<cfcomponent implements="iface_animal">
  <cffunction name="getSound" returntype="string" output="false">
    <cfargument name="volume" type="numeric" required="false" default="5">
    <cfreturn "Woof" & arguments.volume>
  </cffunction>
  <cffunction name="getLegs" returntype="string" output="false">
    <cfreturn "four">
  </cffunction>
</cfcomponent>

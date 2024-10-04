<cfcomponent implements="iface_animal">
  <cffunction name="getSound" returntype="string" output="false">
    <cfargument name="volume" type="numeric" required="true">
    <cfreturn "Woof" & arguments.volume>
  </cffunction>
  <cffunction name="getLegs" returntype="numeric" output="false">
    <cfreturn 4>
  </cffunction>
</cfcomponent>

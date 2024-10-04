<cfinterface displayname="Animal Interface" hint="Interface for animals">
  <cffunction name="getSound" returntype="string" output="false">
    <cfargument name="volume" type="numeric" required="false" default="5">
  </cffunction>
  <cffunction name="getLegs" returntype="numeric" output="false">
  </cffunction>
</cfinterface>

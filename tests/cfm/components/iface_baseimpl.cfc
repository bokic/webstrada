<cfcomponent implements="iface_child">
  <cffunction name="baseMethod" returntype="string" output="false">
    <cfreturn "baseimpl">
  </cffunction>
  <cffunction name="childMethod" returntype="string" output="false">
    <cfreturn "childimpl">
  </cffunction>
</cfcomponent>

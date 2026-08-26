<cfcomponent>
    <cfset this.settings = structNew()>
    <cffunction name="setSettings" output="false">
        <cfargument name="settings" type="struct" required="true">
        <cfset this.settings.skins.path = "base/skins/">
        <cfreturn this.settings.skins.path>
    </cffunction>
</cfcomponent>

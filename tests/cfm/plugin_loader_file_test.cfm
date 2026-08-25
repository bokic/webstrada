<cfset loader = createObject("component", "components.plugin_loader_file")>
<cfoutput>#loader.run("SubscriptionHandler", getDirectoryFromPath(getCurrentTemplatePath()) & "components/plugins/")#</cfoutput>

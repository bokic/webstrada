<cfset blogManager = createObject("component", "components.NestedOwner").init()>
<cfset currentAuthor = blogManager.getCurrentUser()>
<cfoutput>#currentAuthor.currentRole.preferences.get("admin", "menuItems", "all")#</cfoutput>

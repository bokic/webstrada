<cfcomponent>
<cfscript>
this.name = "test_mappings_suite";
this.mappings = structNew();
this.mappings["/org/mangoblog"] = getDirectoryFromPath(getCurrentTemplatePath()) & "components";
</cfscript>
</cfcomponent>

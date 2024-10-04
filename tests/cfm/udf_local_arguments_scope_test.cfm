<!--- CF's `local` scope holds only var-declared names + ARGUMENTS; function
     arguments live in `arguments` (was BUGS.md "Function arguments are exposed
     in the local scope"). Checks use case-insensitive ListFind / isDefined so
     the CF-hash key order and var-key casing do not matter. --->
<cffunction name="f" output="true">
  <cfargument name="arg1">
  <cfargument name="arg2" default="D2">
  <cfset var v1 = "VAR1">
  <cfset localKeys = StructKeyList(local)>
  <cfoutput>#ListFindNoCase(localKeys,"arg1") GT 0#:#ListFindNoCase(localKeys,"v1") GT 0#:#ListFindNoCase(localKeys,"arguments") GT 0#|#arg1#:#arg2#:#v1#|#isDefined("local.arg1")#:#isDefined("local.v1")#:#isDefined("local.arg2")#</cfoutput>
  <cfset arg1 = "MUT">
  <cfoutput>|W:#arguments.arg1#:#arg1#:#isDefined("local.arg1")#</cfoutput>
</cffunction>
<cffunction name="g" output="true">
  <cfargument name="arg1">
  <cfset local.arg1 = "L">
  <cfset arguments.arg1 = "Y">
  <cfset arg1 = "Y2">
  <cfoutput>|S:#local.arg1#:#arguments.arg1#:#arg1#</cfoutput>
</cffunction>
<cffunction name="h" output="true">
  <cfargument name="arg1">
  <cfset local.arg1 = "L">
  <cfset arg1 = "Y">
  <cfoutput>|T:#local.arg1#:#arguments.arg1#:#arg1#</cfoutput>
</cffunction>
<cfoutput>#f("A")##g("A")##h("A")#</cfoutput>

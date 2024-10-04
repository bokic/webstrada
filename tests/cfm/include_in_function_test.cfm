<!--- A <cfinclude> executed inside a <cffunction> reads/writes the function's
     var-declared locals (was BUGS.md "cfinclude inside a function does not
     share the function's local scope"); `variables` still points at the
     calling page, and `arguments` is visible. --->
<cffunction name="g" output="true">
  <cfargument name="arg1">
  <cfset var localVar = "LOCAL">
  <cfset Local.newlocal = "FN">
  <cfinclude template="include_lib/fn_local_share.cfm">
  <cfoutput>[after=#localVar#|newlocal=#Local.newlocal#|pageX=#variables.pageX#|arg=#arguments.arg1#]</cfoutput>
</cffunction>
<cfset pageX = "PAGE">
<cfoutput>#g("A")#|pageX=#pageX#</cfoutput>

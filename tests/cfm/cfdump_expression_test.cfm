<!--- cfdump var="#expr#" compiles the expression, so dotted member chains and
function arguments resolve (previously only a bare variables-scope key worked,
so var="#arguments.obj#" threw "Variable 'arguments.obj' not found in scope for
cfdump"). Verified against CF 2025. --->
<cffunction name="logObject">
	<cfargument name="obj" type="any" required="true">
	<cfsavecontent variable="message">
		<cfdump var="#arguments.obj#">
	</cfsavecontent>
	<cfreturn message>
</cffunction>
<cfset s = {a: {b: "dotted"}}>
<cfoutput>HASDOTTED=#FindNoCase("dotted", logObject(s.a)) GT 0#</cfoutput>
<cfoutput>HASB=#FindNoCase("B", logObject(s.a)) GT 0#</cfoutput>

<cfset c = createObject("component", "components.cfloop_scope_comp") />
<cfoutput>LISTVAR:#c.listVar()#</cfoutput>
<cfoutput>|LISTNONVAR:#c.listNonVar()#</cfoutput>
<cfoutput>|NUMVAR:#c.numericVar()#</cfoutput>
<cfoutput>|NUMNONVAR:#c.numericNonVar()#</cfoutput>
<cfoutput>|ARRVAR:#c.arrayVar()#</cfoutput>
<cfoutput>|COLLVAR:#c.collectionVar()#</cfoutput>
<cfoutput>|SCRIPTVAR:#c.scriptForInVar()#</cfoutput>
<cfoutput>|SCRIPTNONVAR:#c.scriptForInNonVar()#</cfoutput>
<cfset arr = ["alpha","beta"] />
<cfloop array="#arr#" index="pageItem">
	<cfoutput>|TEMPLATE_ARRAY:#pageItem#</cfoutput>
</cfloop>
<cfset tplLeak = "" />
<cffunction name="leakFn" returntype="string">
	<cfset var out = "" />
	<cfloop list="/m/n" index="leakvar" delimiters="/">
		<cfset out = out & "[" & leakvar & "]" />
	</cfloop>
	<cfreturn out />
</cffunction>
<cfoutput>|LEAK:#leakFn()#|isDefined=#isDefined("leakvar")#</cfoutput>

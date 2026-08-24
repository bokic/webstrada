<cffunction name="g">
	<cfargument name="x" required="false" default="DA" />
	<cfreturn structKeyList(arguments) & "=" & arguments.x>
</cffunction>
<cfoutput>#g(zzz="x")#|</cfoutput>
<cfoutput>#g(x="ok")#|</cfoutput>
<cfoutput>#g(zzz="a", x="b")#|</cfoutput>
<cffunction name="f">
	<cfreturn structKeyList(arguments)>
</cffunction>
<cfoutput>#f(a=1, b=2)#|</cfoutput>
<cfoutput>#f(x=1)#|</cfoutput>
<cfoutput>#f("p","q")#</cfoutput>

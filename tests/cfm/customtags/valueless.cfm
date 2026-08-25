<cfoutput>[keys=#structKeyList(attributes)#]</cfoutput>
<cfif structKeyExists(attributes, "a")>
	<cfoutput>[a=#attributes.a#][aLen=#len(attributes.a)#]</cfoutput>
</cfif>
<cfif structKeyExists(attributes, "b")>
	<cfoutput>[b=#attributes.b#]</cfoutput>
</cfif>
<cfif structKeyExists(attributes, "charset")>
	<cfoutput>[charset=#attributes.charset#][charsetLen=#len(attributes.charset)#]</cfoutput>
<cfif attributes.charset>
	<cfoutput>[charsetTruthy=YES]</cfoutput>
</cfif>
</cfif>

<cfoutput>[Mkeys=#structKeyList(attributes)#]</cfoutput>
<cfif structKeyExists(attributes, "a")>
	<cfoutput>[Ma=#attributes.a#][MaLen=#len(attributes.a)#]</cfoutput>
</cfif>
<cfif structKeyExists(attributes, "b")>
	<cfoutput>[Mb=#attributes.b#]</cfoutput>
</cfif>
<cfif structKeyExists(attributes, "charset")>
	<cfoutput>[Mcharset=#attributes.charset#][McharsetLen=#len(attributes.charset)#]</cfoutput>
<cfif attributes.charset>
	<cfoutput>[McharsetTruthy=YES]</cfoutput>
</cfif>
</cfif>

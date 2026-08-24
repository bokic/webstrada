<cffunction name="getSetting">
	<cfargument name="key" type="any" required="true" />
	<cfreturn "1" />
</cffunction>
<cfset x = 2 />
<cfswitch expression="#getSetting("confirmationMethod")#">
	<cfcase value="1">
		<cfoutput>ONE</cfoutput>
	</cfcase>
	<cfcase value="2">
		<cfoutput>TWO</cfoutput>
	</cfcase>
</cfswitch>
<cfoutput>|</cfoutput>
<cfset y = getSetting("confirmationMethod") />
<cfoutput>#y#|DONE</cfoutput>

<cffunction name="initSettings">
	<cfset var key = "" />
	<cfset variables.settings = StructNew() />
	<cfloop collection="#arguments#" item="key">
		<cfset variables.settings[key] = arguments[key] />
	</cfloop>
</cffunction>
<cfset initSettings(paragraphComments=true, paragraphPosts=false, paragraphPages=false,
					htmlFormatComments=true, htmlFormatPosts=false, htmlFormatPages=false) />
<cfoutput>#variables.settings.paragraphComments#|#variables.settings.htmlFormatComments#|#variables.settings.htmlFormatPages#|#structCount(variables.settings)#</cfoutput>

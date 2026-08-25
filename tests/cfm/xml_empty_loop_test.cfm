<cfset helper = new components.xml_empty_loop() />
<cfoutput>#helper.countPageTemplates("<skin><pageTemplates></pageTemplates></skin>")#</cfoutput>

<!--- <cfflush> with an empty buffer commits the response (was BUGS.md #8) --->
<cfcontent type="text/html; charset=ISO-8859-1">
<cfflush>
<cfcontent type="text/html; charset=UTF-8">
<cfoutput>#Chr(233)#</cfoutput>

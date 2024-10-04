<cfset d = CreateDate(2020,5,15)>
<cfset SetLocale("Italian (Standard)")>
<cfoutput>
#LSDateFormat(d, "mmmm")#|#LSDateFormat(d, "MMMM")#|#LSDateFormat(d, "d mmmm yyyy")#|#LSDateFormat(d, "mmmm yyyy")#|#LSDateFormat(d, "mmmm d")#|
</cfoutput>
<cfset SetLocale("French (Standard)")>
<cfoutput>
#LSDateFormat(d, "mmmm")#|
</cfoutput>
<cfset SetLocale("Spanish (Standard)")>
<cfoutput>
#LSDateFormat(d, "mmmm")#
</cfoutput>

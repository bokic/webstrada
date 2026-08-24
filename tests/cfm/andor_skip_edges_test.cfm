<cfset a = false />
<cfoutput>#(a AND (undefinedVar EQ 1))#|</cfoutput>
<cfoutput>#(true OR [1,2,3][5])#|</cfoutput>
<cfoutput>#(false AND "x" & "y")#|</cfoutput>
<cfoutput>#(true OR 1 GT 2)#|</cfoutput>
<cfoutput>#(a OR "fallback")#</cfoutput>

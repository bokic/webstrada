<cfset x = 1/3>
<cfset y = 2^53>
<cfset w = 10^9>
<cfset v = 10^-5>
<cfset u = 10^7>
<cfset t = 1234567.5>
<cfset q = 10^14>
<cfset r = 1/100000>
<cfset z = 3.14>
<cfset arr = ArrayNew(1)>
<cfset ArrayAppend(arr, 8.0)>
<cfset ArrayAppend(arr, x)>
<cfoutput>#SerializeJSON(x)#|#SerializeJSON(y)#|#SerializeJSON(w)#|#SerializeJSON(v)#|#SerializeJSON(u)#|#SerializeJSON(t)#|#SerializeJSON(q)#|#SerializeJSON(r)#|#SerializeJSON(z)#|#SerializeJSON(arr)#</cfoutput>

<cfset s={a={b={c="w"}}}>
<cfset k="a">
<cfoutput>A:#s . a . b . c #</cfoutput>
<cfoutput>B:#s[k]. b . c #</cfoutput>
<cfoutput>C:#s.a.b.c#</cfoutput>
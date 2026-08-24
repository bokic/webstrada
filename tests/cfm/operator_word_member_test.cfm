<!--- Operator keywords used as struct property names (contains, eq, mod, and,
not, or, gt, lt, is) must resolve as members after a dot, never as the CFML
operator. Verified against CF 2025; see also OperatorWordAsMemberTest. --->
<cfset s = {contains: 5, eq: 6, mod: 7, not: 8, and: 9, or: 10, gt: 11, lt: 12, is: 13}>
<cfset s2 = {inner: {contains: 42}}>
<cfset s3 = {contains: 5, adminmode: false}>
<cfset s4 = {a: true}>
<cfset b = false>
<cfoutput>
P1=#s.contains#|#s.eq#|#s.mod#|#s.not#|#s.and#|#s.or#|#s.gt#|#s.lt#|#s.is#
P2=#s2.inner.contains#
P3=#s.contains#
</cfoutput>
<cfif NOT s3.adminmode AND s3.contains><cfoutput>M=Y</cfoutput><cfelse><cfoutput>M=N</cfoutput></cfif>
<cfif s4.a AND (b EQ false)><cfoutput>G=Y</cfoutput><cfelse><cfoutput>G=N</cfoutput></cfif>
<cfif "hello world" contains "wor"><cfoutput>C=Y</cfoutput><cfelse><cfoutput>C=N</cfoutput></cfif>
<cfif 10 GT 5 AND 2 LT 3 AND NOT false><cfoutput>O=Y</cfoutput><cfelse><cfoutput>O=N</cfoutput></cfif>
<cfscript>
sx = {mod: 7, contains: 5};
writeOutput("S=" & sx.mod & "|" & sx.contains & "#chr(10)#");
</cfscript>
<cfset a = [1,2,3]>
<cfoutput>M2=#a.contains(2)#|#a.contains(9)#</cfoutput>

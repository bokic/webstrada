<cfoutput>START|</cfoutput>
<cfimport path="components.mylib.subcomp">
<cfset c = CreateObject("component", "subcomp")>
<cfoutput>#c.sub()#|</cfoutput>
<cfimport path="components.mylib.*">
<cfset c2 = CreateObject("component", "subcomp")>
<cfoutput>#c2.sub()#|</cfoutput>
<cfset c3 = new subcomp()>
<cfoutput>#c3.sub()#</cfoutput>

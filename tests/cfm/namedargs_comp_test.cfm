<cfset c = createObject("component", "components.named_calc_extra") />
<cfoutput>#c.greet(zzz="x")#|</cfoutput>
<cfoutput>#c.greet(who="ok")#|</cfoutput>
<cfoutput>#c.greet(y="1", who="2")#|</cfoutput>
<cfoutput>#c.noparam(a=1, b=2)#</cfoutput>

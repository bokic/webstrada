<cfoutput>START;</cfoutput>
<cfset p = new components.person()>
<cfoutput>#p.getName()#|#p.getSpecies()#|</cfoutput>
<cfset p2 = new components.person("Zoe")>
<cfoutput>#p2.getName()#|</cfoutput>
<cfset c = new components.circle()>
<cfoutput>#c.describe()#|#c.area()#|</cfoutput>
<cfset c2 = new components.circle("green", 2)>
<cfoutput>#c2.describe()#|#c2.area()#|</cfoutput>

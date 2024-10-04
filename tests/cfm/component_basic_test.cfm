<cfoutput>START;</cfoutput>
<cfset p = CreateObject("component", "components/person")>
<cfset p.init("Alice")>
<cfoutput>#p.getName()#|#p.getSpecies()#|#p.greet("Bob")#|</cfoutput>
<cfset p.setName("Carol")>
<cfoutput>#p.name#|#p.add(20, 22)#|</cfoutput>
<cfoutput>#IsObject(p)#|#IsStruct(p)#|#IsSimpleValue(p)#|</cfoutput>
<cftry><cfoutput>#p.secret()#</cfoutput><cfcatch><cfoutput>PRIV</cfoutput></cfcatch></cftry>
<cfoutput>|#p.reveal()#|</cfoutput>
<cfoutput>|#StructKeyExists(p, "getName")#|#StructKeyExists(p, "secret")#|</cfoutput>
<cfoutput>|#SerializeJSON(p)#</cfoutput>
<cfoutput>|#p.echoThis().getName()#|</cfoutput>

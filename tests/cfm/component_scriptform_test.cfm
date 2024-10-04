<cfoutput>START;</cfoutput>
<cfset p = CreateObject("component", "components/modern_person")>
<cfset p.init("Alice")>
<cfoutput>#p.getName()#|#p.appName#|#p.version#|#p.greet()#|#p.greet("Bob")#|#p.add(20, 22)#|</cfoutput>
<cfoutput>#IsObject(p)#|#IsStruct(p)#|#IsSimpleValue(p)#|</cfoutput>
<cftry><cfoutput>#p.secret()#</cfoutput><cfcatch><cfoutput>PRIV</cfoutput></cfcatch></cftry>
<cfoutput>|#p.reveal()#|</cfoutput>
<cfoutput>|#StructKeyExists(p, "getName")#|#StructKeyExists(p, "secret")#|</cfoutput>
<cfoutput>|#ListSort(StructKeyList(p), "text")#|</cfoutput>
<cfoutput>|#p.echoThis().getName()#|</cfoutput>
<cfset sc = CreateObject("component", "components/script_comment")>
<cfset sc2 = CreateObject("component", "components/script_comment2")>
<cfoutput>|#sc.fromComment()#|#sc2.fromComment()#|</cfoutput>

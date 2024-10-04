<cfoutput>START;</cfoutput>
<cfset p = new components.person()>
<cfoutput>#SerializeJSON(p)#|</cfoutput>
<cfoutput>#ListSort(StructKeyList(p), "text")#|</cfoutput>
<cfset p.firstProp = "set-from-outside">
<cfoutput>#p.firstProp#|#StructKeyExists(p, "firstprop")#|</cfoutput>

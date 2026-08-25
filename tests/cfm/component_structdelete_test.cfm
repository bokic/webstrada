<cfset c = CreateObject("component", "components.structdelete_comp")>
<cfoutput>#c.remove()#|#StructKeyExists(c, "toRemove")#</cfoutput>
